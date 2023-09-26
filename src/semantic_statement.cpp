//

#include "semantic_statement.h"
#include "bitwise_functions.h"
#include "syntax.h"
#include "templates.h"
#include <algorithm>

//
// Created by djordy on 12/11/22.

uint word_size(binary_data::allignment_t allignment) { return 1 << (uint) allignment; }

semantic_statements::data_statement::data_statement(const syntax::inst_statement &inst_stmnt,
                                                    asm_lang::data_statement_id id) {

    this->id = id;
    const auto &args = inst_stmnt.args;

    //---argument 1 (register)---//
    if (set_register(args[0], operand1) == false)
        throw std::runtime_error("expected register as first operand");

    //---argument 2 (register)---//
    if (set_register(args[1], reg_location)) {
        has_label_operand_m = false;

        //---optional argument 3 (immediate offset)---/
        if (args.size() == 3) {
            if (set_immediate(args[2], reg_location_offset) == false)
                throw std::runtime_error("expected integer as third operand");
        } else
            reg_location_offset = 0;
        //no integer operand is implicit zero jump_label

        //---argument 2 (label)---//
    } else if (set_label_operand(args[1], label_location)) {
        has_label_operand_m = true;

        //---optional argument 3 (immediate label offset)---//
        if (args.size() == 3)
            if (set_label_operand_offset(args[2], label_location) == false)
                throw std::runtime_error("expected integer as third operand");
    } else {
        throw std::runtime_error("expected register or label as second operand");
    }
}

int semantic_statements::data_statement::get_compile_case(const symbol_table &st, uint32_t pc,
                                                          const program_options &options) const {
    if (has_label_operand_m) {
        //label destination
        const auto symbol_id = label_location.label.symbol_id;
        if (symbol_id == 0) return (int) compile_case_t::undetermined;
        return (int) compile_case_t::label;
    }


    //if not label address must be register destination case
    const uint offset_bitsize = signed_bitwidth(reg_location_offset);
    //check if immediate jump_label fits in a single data instruction (branch format)
    if (offset_bitsize <= isa::branch_lowerimmediate_bitsize + isa::branch_upperimmediate_bitsize)
        return (int) compile_case_t::short_reg;
    else
        return (int) compile_case_t::long_reg;

    assert(!"unreachable");
}

binary_data::memory_alloc_t semantic_statements::data_statement::get_size(int comp_case_int) const {
    const auto comp_case = (compile_case_t) comp_case_int;

    switch (comp_case) {
        case compile_case_t::undetermined:
        case compile_case_t::long_reg:
            return {.nbytes = 12, .allignment = binary_data::allignment_t::word};

        case compile_case_t::label:
            return {.nbytes = 8, .allignment = binary_data::allignment_t::word};

        case compile_case_t::short_reg:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};
    }

    assert(!"unreachable");
}

const static enum_lut<asm_lang::data_statement_id, std::pair<isa::inst_id, bool>>
        data_stmnt_id_lut({
                {asm_lang::data_statement_id::Sw, {isa::inst_id::Sw, false}},
                {asm_lang::data_statement_id::Sh, {isa::inst_id::Sh, false}},
                {asm_lang::data_statement_id::Sb, {isa::inst_id::Sb, false}},
                {asm_lang::data_statement_id::Lw, {isa::inst_id::Lw, true}},
                {asm_lang::data_statement_id::Lh, {isa::inst_id::Lh, true}},
                {asm_lang::data_statement_id::Lb, {isa::inst_id::Lb, true}},
                {asm_lang::data_statement_id::Lhu, {isa::inst_id::Lhu, true}},
                {asm_lang::data_statement_id::Lbu, {isa::inst_id::Lbu, true}},
        });

std::vector<isa::instruction>
semantic_statements::data_statement::gen_instructions(int comp_case_int, const symbol_table &st,
                                                      uint32_t pc) const {
    const auto comp_case = (compile_case_t) comp_case_int;
    assert(comp_case != compile_case_t::undetermined);

    //---determine load/store instruction---//
    const auto &result = data_stmnt_id_lut[id];
    const isa::inst_id instruction_id = result.first;
    auto(*const make_data_inst) = result.second ? isa::make_load_inst : isa::make_store_inst;

    //---generate instructions---//
    switch (comp_case) {
        case compile_case_t::short_reg:
            return {make_data_inst(instruction_id, operand1, reg_location, reg_location_offset)};

        case compile_case_t::long_reg: {
            //example of long store statement, long load statement is done the same, but with a load instruction at the end instead of a store.
            //desired operation: `mem(destination_address_reg + address_offset[31:0]) <-sr1`
            //is equivalent to the sequence of the following instructions:
            //sui: `ar <- address_offset[31:11]`
            //add: `ar <- ar + destination_address_reg`
            //data: `mem(ar + address_offset[10:0]) <- sr`

            //---set lower and upper parts of immediate---//
            const int32_t upper_imm =
                    bitwize_select(reg_location_offset, 32 - isa::set_immediate_bitsize,
                                   isa::set_immediate_bitsize);//address_offset[31:11]
            const int32_t lower_imm =
                    bitwize_select((int32_t) reg_location_offset, 0,
                                   32 - isa::set_immediate_bitsize);//address_offset[10:0]

            //---return instructions---//
            return {isa::make_set_inst(isa::inst_id::Sui, isa::reg_id::ar, upper_imm),
                    isa::make_reg_inst(isa::inst_id::Add, isa::reg_id::ar, isa::reg_id::ar,
                                       reg_location),
                    make_data_inst(instruction_id, operand1, isa::reg_id::ar, lower_imm)};
        }

        case compile_case_t::label: {
            const auto symbol_id = label_location.label.symbol_id;
            assert(symbol_id != 0);
            const auto &symbol = st.get_symbol(symbol_id);
            const auto address_offset = symbol.address + label_location.offset;

            //---set lower and upper parts of immediate---//
            const int32_t upper_imm =
                    bitwize_select(address_offset, 32 - isa::set_immediate_bitsize,
                                   isa::set_immediate_bitsize);//address_offset[31:11]
            const int32_t lower_imm =
                    bitwize_select(address_offset, 0,
                                   32 - isa::set_immediate_bitsize);//address_offset[10:0]

            //---return instructions---//
            return {isa::make_set_inst(isa::inst_id::Sui, isa::reg_id::ar, upper_imm),
                    make_data_inst(instruction_id, operand1, isa::reg_id::ar, lower_imm)};
        }
    }

    assert(!"unreachable");
}

binary::reloc_type
semantic_statements::data_statement::get_reloc_type(int comp_case_int,
                                                    const symbol_table &st) const {
    if (has_label_operand() == false) return binary::reloc_type::none;

    //---symbol look up---//
    const auto symbol_id = label_location.label.symbol_id;
    assert(symbol_id != 0);
    const auto &symbol = st[symbol_id];

    //---get access type---//
    const bool is_load = data_stmnt_id_lut[id].second;

    //---return reloc type---//
    switch (symbol.scope) {
        case symbol_scope::external:
            if (is_load) return binary::reloc_type::symr_long_load;

            return binary::reloc_type::symr_long_store;
        case symbol_scope::global:
        case symbol_scope::local:
            if (is_load) return binary::reloc_type::secr_long_load;

            return binary::reloc_type::secr_long_store;
    }

    assert(!"unreachable");
}

semantic_statements::jump_statement::jump_statement(const syntax::inst_statement &inst_stmnt,
                                                    asm_lang::jump_statement_id id) {
    this->id = id;

    if (inst_stmnt.args.size() == 1) {
        //---argument 1---//
        if (set_register(inst_stmnt.args[0], destination_reg)) {
            dest_type = destination_types::reg;
        } else if (set_label_operand(inst_stmnt.args[0], offset)) {
            dest_type = destination_types::address;
        } else {
            throw std::runtime_error("expected label or register");
        }

        //set implicit return register operand
        if (id == asm_lang::jump_statement_id::Jmp) {
            return_reg = isa::reg_id::zero;
        } else {
            return_reg = isa::reg_id::ra;
        }

    } else if (inst_stmnt.args.size() == 2) {
        if (id == asm_lang::jump_statement_id::Jmp)
            throw std::runtime_error("Jmp statement can only take one operand");

        //---argument 1---//
        if (set_register(inst_stmnt.args[0], return_reg) == false)
            throw std::runtime_error("expected register");

        //---argument 2---//
        if (set_register(inst_stmnt.args[1], destination_reg)) {
            dest_type = destination_types::reg;
        } else if (set_label_operand(inst_stmnt.args[1], offset)) {
            dest_type = destination_types::address;
        } else {
            throw std::runtime_error("expected label, register or immediate");
        }
    } else {
        throw std::runtime_error("wrong number of arguments");
    }
}

int semantic_statements::jump_statement::get_compile_case(const symbol_table &st, uint32_t pc,
                                                          const program_options &options) const {
    //register destination
    if (dest_type == destination_types::reg) return (int) compile_case_t::reg_jump;

    //---symbol lookup---//
    const auto symbol_id = offset.label.symbol_id;
    const auto &symbol = st[symbol_id];

    //undetermined
    if (symbol_id == 0) return (int) compile_case_t::undetermined;

    //---external symbol case---//
    const bool ra_return_reg = (return_reg == isa::reg_id::ra);
    const bool zero_return_reg = (return_reg == isa::reg_id::zero);
    const bool is_external_jmp = symbol.scope == symbol_scope::external;

    if (is_external_jmp) {
        if (options.short_jumps && ra_return_reg) return (int) compile_case_t::short_ra_jump;
        if (options.short_jumps && zero_return_reg) return (int) compile_case_t::short_no_jump;
        return (int) compile_case_t::full_jump;
    }

    //must be local or global symbol
    assert(symbol.scope == symbol_scope::global || symbol.scope == symbol_scope::local);

    if (symbol.type == symbol_type::data) throw std::runtime_error("expected executable label");

    const int32_t address = symbol.address - pc;
    const auto offset_bitwidth = signed_bitwidth(address) - 1;
    //the least significant bit of the address is always zero,
    //the immediate operand in a jump instructions doesn't include this least significant bit, therefore 1 is subtracted from the bitwidth

    if (offset_bitwidth <= isa::jump_immediate_bitsize) {
        if (zero_return_reg) return (int) compile_case_t::short_no_jump;
        if (ra_return_reg) return (int) compile_case_t::short_ra_jump;
    }

    return (int) compile_case_t::full_jump;
}

binary_data::memory_alloc_t semantic_statements::jump_statement::get_size(int comp_case_int) const {
    const auto comp_case = (compile_case_t) comp_case_int;

    switch (comp_case) {
        case compile_case_t::undetermined:
        case compile_case_t::full_jump:
            return {.nbytes = 8, .allignment = binary_data::allignment_t::word};

        case compile_case_t::short_no_jump:
        case compile_case_t::short_ra_jump:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};

        case compile_case_t::reg_jump:
            return {.nbytes = 2, .allignment = binary_data::allignment_t::halfword};
    }

    assert(!"unreachable");
}

std::vector<isa::instruction>
semantic_statements::jump_statement::gen_instructions(int comp_case_int, const symbol_table &st,
                                                      uint32_t pc) const {
    const auto comp_case = (compile_case_t) comp_case_int;
    assert(comp_case != compile_case_t::undetermined);

    if (comp_case == compile_case_t::reg_jump)
        return {isa::make_half_reg_inst(isa::inst_id::Jalr_h, return_reg, destination_reg)};

    //must be label jump address now
    //---symbol lookup---//
    const auto symbol_id = offset.label.symbol_id;
    assert(symbol_id != 0);
    const auto &symbol = st.get_symbol(symbol_id);
    assert(symbol.type != symbol_type::data);

    const int32_t jump_offset = (symbol.scope != symbol_scope::external) ? symbol.address - pc : 0;

    switch (comp_case) {
        case compile_case_t::full_jump: {
            const uint32_t upper_int = bitwize_select(jump_offset, 32 - isa::set_immediate_bitsize,
                                                      isa::set_immediate_bitsize);
            const uint32_t lower_int =
                    bitwize_select(jump_offset, 0, 32 - isa::set_immediate_bitsize);

            return {isa::make_set_inst(isa::inst_id::Apci, isa::reg_id::ar, upper_int),
                    isa::make_immediate_inst(isa::inst_id::Jalr, return_reg, isa::reg_id::ar,
                                             lower_int)};
        }

        case compile_case_t::short_no_jump:
            return {isa::make_jump_inst(isa::inst_id::Rji, jump_offset)};

        case compile_case_t::short_ra_jump:
            return {isa::make_jump_inst(isa::inst_id::Rjali, jump_offset)};
    }

    assert(!"unreachable");
}

binary::reloc_type
semantic_statements::jump_statement::get_reloc_type(int comp_case_int,
                                                    const symbol_table &st) const {
    if (has_label_operand() == false) return binary::reloc_type::none;

    //must be label location jump
    //symbol lookup
    const auto symbol_id = offset.label.symbol_id;
    assert(symbol_id != 0);
    const auto &symbol = st.get_symbol(symbol_id);

    const auto comp_case = (compile_case_t) comp_case_int;
    switch (symbol.scope) {
        case symbol_scope::external:
            if (comp_case == compile_case_t::short_ra_jump ||
                comp_case == compile_case_t::short_no_jump)
                return binary::reloc_type::short_jump;

            return binary::reloc_type::long_jump;
        case symbol_scope::local:
        case symbol_scope::global:
            return binary::reloc_type::dummy;
    }

    assert(!"unreachable");
}

semantic_statements::branch_statement::branch_statement(const syntax::inst_statement &inst_stmnt,
                                                        asm_lang::branch_statement_id id) {
    this->id = id;
    if (inst_stmnt.args.size() != 3) throw std::runtime_error("wrong number of arguments");

    //---argument 1---//
    if (set_register(inst_stmnt.args[0], operand1) == false)
        throw std::runtime_error("Expected register as first argument");

    //---argument 2---//
    if (set_register(inst_stmnt.args[1], operand2) == false)
        throw std::runtime_error("Expected register as second argument");

    //---argument 3---//
    if (set_label_operand(inst_stmnt.args[2], jump_label) == false)
        throw std::runtime_error("expected label as third argument");
}

int semantic_statements::branch_statement::get_compile_case(const symbol_table &st, uint32_t pc,
                                                            const program_options &options) const {
    //---symbol lookup---//
    const auto symbol_id = jump_label.label.symbol_id;
    if (symbol_id == 0) return (int) compile_case_t::undetermined;

    const auto &symbol = st.get_symbol(symbol_id);
    if (symbol.type != symbol_type::fun) throw std::runtime_error("operand is not function label");

    //if external symbol
    if (symbol.scope == symbol_scope::external)
        throw std::runtime_error("cannot branch to external symbol");


    const int32_t jump_offset = symbol.address - pc;
    const int32_t worst_casa_address = (jump_offset < 0) ? jump_offset - 4 : jump_offset;

    //the least significant bit of the address is always zero,
    //the immediate operand in a branch instructions doesn't include this least significant bit, so 1 is subtracted from the needed bitwidth
    const auto offset_bitwidth = signed_bitwidth(worst_casa_address) - 1;

    if (offset_bitwidth <=
        isa::branch_lowerimmediate_bitsize + isa::branch_lowerimmediate_bitsize) {
        return (int) compile_case_t::short_branch;
    }

    return (int) compile_case_t::long_branch;
}

binary_data::memory_alloc_t
semantic_statements::branch_statement::get_size(int comp_case_int) const {
    const auto comp_case = (compile_case_t) comp_case_int;

    switch (comp_case) {
        case compile_case_t::undetermined:
        case compile_case_t::long_branch:
            return {.nbytes = 8, .allignment = binary_data::allignment_t::word};
        case compile_case_t::short_branch:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};
    }

    assert(!"unreachable");
}

const enum_lut<asm_lang::branch_statement_id, isa::inst_id>
        branch_stmnt_inst_id_lut({{asm_lang::branch_statement_id::Beq, isa::inst_id::Beq},
                                  {asm_lang::branch_statement_id::Bne, isa::inst_id::Bne},
                                  {asm_lang::branch_statement_id::Bgr, isa::inst_id::Bgr},
                                  {asm_lang::branch_statement_id::Bge, isa::inst_id::Bge},
                                  {asm_lang::branch_statement_id::Bgru, isa::inst_id::Bgru},
                                  {asm_lang::branch_statement_id::Bgeu, isa::inst_id::Bgeu}});

const enum_lut<asm_lang::branch_statement_id, isa::inst_id> branch_stmnt_reverse_inst_id_lut(
        {{asm_lang::branch_statement_id::Beq, isa::inst_id::Bne},
         {asm_lang::branch_statement_id::Bne, isa::inst_id::Beq},
         {asm_lang::branch_statement_id::Bgr, isa::inst_id::Bge},
         {asm_lang::branch_statement_id::Bge, isa::inst_id::Bgr},
         {asm_lang::branch_statement_id::Bgru, isa::inst_id::Bgeu},
         {asm_lang::branch_statement_id::Bgeu, isa::inst_id::Bgru}});

std::vector<isa::instruction>
semantic_statements::branch_statement::gen_instructions(int comp_case_int, const symbol_table &st,
                                                        uint32_t pc) const {
    const auto comp_case = (compile_case_t) comp_case_int;
    assert(comp_case != compile_case_t::undetermined);

    //---symbol lookup---//
    const auto symbol_id = jump_label.label.symbol_id;
    assert(symbol_id != 0);
    const auto &symbol = st[symbol_id];
    assert(symbol.scope != symbol_scope::external);
    const int32_t jump_offset = symbol.address - pc;

    //---return compile case---//
    switch (comp_case) {
        case compile_case_t::long_branch: {
            //branch instruction has to branch over a jump instruction
            //therefore the branch instructions needs to have the reversed logical condition of the assembly branch statement
            //This is done by swapping Greater Equal with Greater, Not Equal with Equal and swapping the register operands
            //Example !(a > b) = (a <= b) = (b >= a)
            //Example !(a >= b) = (a < b) = (b > a)
            //Example !(a == b) = (a != b) = (b != a) here register swapping is not necessary but is done anyway for consistency

            const isa::inst_id branch_inst_id = branch_stmnt_reverse_inst_id_lut[id];
            return {isa::make_branch_inst(branch_inst_id, operand2, operand1, 4),
                    isa::make_jump_inst(isa::inst_id::Rji, (jump_offset - 4))};
            //jump instruction is places 4 bytes further than then what the offset is calculated for, these 4 bytes have to be subtracted
        }

        case compile_case_t::short_branch: {
            const isa::inst_id branch_inst_id = branch_stmnt_inst_id_lut[id];
            return {isa::make_branch_inst(branch_inst_id, operand1, operand2, jump_offset)};
        }
    }

    assert(!"unreachable");
}

semantic_statements::unary_statement::unary_statement(const syntax::inst_statement &inst_stmnt,
                                                      asm_lang::unary_statement_id id) {
    this->id = id;

    const auto &args = inst_stmnt.args;
    if (args.size() != 1 && args.size() != 2)
        throw std::runtime_error("incorrect number of arguments");

    /*---argument 1---*/
    if (set_register(args[0], destination) == false)
        throw std::runtime_error("argument is not register");

    if (args.size() == 1) {
        operand = destination;
    } else {
        /*---argument 2---*/
        if (set_register(args[1], operand) == false)
            throw std::runtime_error("argument is not register");
    }
}

int semantic_statements::unary_statement::get_compile_case(const symbol_table &st, uint32_t pc,
                                                           const program_options &options) const {
    switch (id) {
        case asm_lang::unary_statement_id::Neg:
            return (int) compile_case_t::full_reg;

        case asm_lang::unary_statement_id::Not:
            if (destination == operand) return (int) compile_case_t::half_reg;

            return (int) compile_case_t::full_reg;
    }

    assert(!"unreachable");
}

binary_data::memory_alloc_t
semantic_statements::unary_statement::get_size(int comp_case_int) const {
    const auto comp_case = (compile_case_t) comp_case_int;

    switch (comp_case) {
        case compile_case_t::full_reg:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};

        case compile_case_t::half_reg:
            return {.nbytes = 2, .allignment = binary_data::allignment_t::halfword};
    }

    assert(!"unreachable");
}

std::vector<isa::instruction>
semantic_statements::unary_statement::gen_instructions(int comp_case_int, const symbol_table &st,
                                                       uint32_t pc) const {
    auto comp_case = (compile_case_t) comp_case_int;
    assert(comp_case != compile_case_t::undetermined);

    switch (comp_case) {
        case compile_case_t::full_reg:
            switch (id) {
                case asm_lang::unary_statement_id::Not:
                    return {isa::make_reg_inst(isa::inst_id::Xnor, destination, isa::reg_id::zero,
                                               operand)};

                case asm_lang::unary_statement_id::Neg:
                    return {isa::make_reg_inst(isa::inst_id::Sub, destination, isa::reg_id::zero,
                                               operand)};
            }

            break;
        case compile_case_t::half_reg:
            switch (id) {
                case asm_lang::unary_statement_id::Not:
                    return {isa::make_half_reg_inst(isa::inst_id::Nand_h, destination, operand)};
            }
            break;
    }

    assert(!"unreachable");
}

semantic_statements::set_statement::set_statement(const syntax::inst_statement &inst_stmnt,
                                                  asm_lang::set_statement_id) {
    this->id = id;
    const auto &args = inst_stmnt.args;

    if (args.size() != 2) throw std::runtime_error("Wrong number of arguments");

    //---argument 1 (register)---//
    if (set_register(args[0], destination_reg) == false)
        throw std::runtime_error("first argument is not a register");

    //---argument 2 (integer)---//
    if (set_immediate(args[1], source_integer)) {
        src_type = source_types::integer;

        //---argument 2 (label)---//
    } else if (set_label_operand(args[1], source_address)) {
        src_type = source_types::address_label;

        //---argument 3 (optional integer offset)---//
        if (args.size() == 3)
            if (set_label_operand_offset(args[2], source_address) == false)
                throw std::runtime_error("expected third integer operand");

        //---argument 2 (register)---//
    } else if (set_register(args[1], source_reg)) {
        src_type = source_types::reg;
    } else
        throw std::runtime_error("second argument is neither label nor immediate");
}

int semantic_statements::set_statement::get_compile_case(const symbol_table &st, uint32_t pc,
                                                         const program_options &options) const {
    switch (src_type) {
        case source_types::integer:
            if (signed_bitwidth(source_integer) <= isa::imm_immediate_bitsize)
                return (int) compile_case_t::int_lower_fit;
            else if (bitwize_select(source_integer, 0, 32 - isa::set_immediate_bitsize) == 0)
                return (int) compile_case_t::int_upper_fit;
            else
                return (int) compile_case_t::int_full;

        case source_types::reg:
            return (int) compile_case_t::reg_mov;

        case source_types::address_label: {
            const auto symbol_id = source_address.label.symbol_id;
            //---undetermined case---//
            if (symbol_id == 0) return (int) compile_case_t::undetermined;
            const auto &symbol = st.get_symbol(symbol_id);

            //---data label---//
            if (symbol.section != binary::section_t::text) return (int) compile_case_t::data_label;

            //---pc relative---//
            //if not data label must be pc relative label in text section
            return (int) compile_case_t::pc_rel;
        }
    }

    assert(!"unreachable");
}

binary_data::memory_alloc_t semantic_statements::set_statement::get_size(int comp_case_int) const {
    auto comp_case = (compile_case_t) comp_case_int;

    switch (comp_case) {
        case compile_case_t::int_lower_fit:
        case compile_case_t::int_upper_fit:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};

        case compile_case_t::reg_mov:
            return {.nbytes = 2, .allignment = binary_data::allignment_t::halfword};

        case compile_case_t::undetermined:
        case compile_case_t::int_full:
        case compile_case_t::data_label:
        case compile_case_t::pc_rel:
            return {.nbytes = 8, .allignment = binary_data::allignment_t::word};
        default:
            assert(false);
    }
}


std::vector<isa::instruction>
semantic_statements::set_statement::gen_instructions(int comp_case_int, const symbol_table &st,
                                                     uint32_t pc) const {
    const auto comp_case = (compile_case_t) comp_case_int;
    assert(comp_case != compile_case_t::undetermined);

    //---set integer value---//
    uint32_t integer_val = 0;
    if (src_type == source_types::address_label) {
        //---symbol look up---//
        const auto symbol_id = source_address.label.symbol_id;
        assert(symbol_id != 0);
        const auto &symbol = st.get_symbol(symbol_id);
        integer_val = source_address.offset + symbol.address;
        if (comp_case == compile_case_t::pc_rel) integer_val -= pc;
    } else {
        integer_val = source_integer;
    }

    const auto lower_int =
            bitwize_select((int32_t) integer_val, 0, 32 - isa::set_immediate_bitsize);
    const auto upper_int = bitwize_select((int32_t) integer_val, 32 - isa::set_immediate_bitsize,
                                          isa::set_immediate_bitsize);

    //---generate instructions by compile case---//
    switch (comp_case) {
        case compile_case_t::pc_rel:
            return {
                    isa::make_set_inst(isa::inst_id::Apci, destination_reg, upper_int),
                    isa::make_immediate_inst(isa::inst_id::Addi, destination_reg, destination_reg,
                                             lower_int),
            };

        case compile_case_t::reg_mov:
            return {isa::make_half_reg_inst(isa::inst_id::Mov, destination_reg, source_reg)};

        case compile_case_t::data_label:
        case compile_case_t::int_full:
            return {
                    isa::make_set_inst(isa::inst_id::Sui, destination_reg, upper_int),
                    isa::make_immediate_inst(isa::inst_id::Addi, destination_reg, destination_reg,
                                             lower_int),
            };

        case compile_case_t::int_upper_fit:
            return {isa::make_set_inst(isa::inst_id::Sui, destination_reg, upper_int)};

        case compile_case_t::int_lower_fit:
            return {isa::make_set_inst(isa::inst_id::Sli, destination_reg, source_integer)};
    }

    assert(!"unreachable");
}

binary::reloc_type
semantic_statements::set_statement::get_reloc_type(int comp_case_int,
                                                   const symbol_table &st) const {

    const auto &symbol = st.get_symbol(source_address.label.symbol_id);

    const auto comp_case = (compile_case_t) comp_case_int;
    switch (comp_case) {
        case compile_case_t::data_label:
            if (symbol.scope == symbol_scope::external) return binary::reloc_type::symr_long_store;
            return binary::reloc_type::secr_long_store;

        case compile_case_t::pc_rel:
            if (symbol.scope == symbol_scope::external) return binary::reloc_type::long_jump;
            return binary::reloc_type::dummy;
    }

    return binary::reloc_type::none;
}

semantic_statements::reg_arith_statement::reg_arith_statement(
        const syntax::inst_statement &inst_stmnt, asm_lang::reg_arith_statement_id id) {
    this->id = id;
    const auto &args = inst_stmnt.args;

    if (args.size() == 2) {
        /*---argument 1---*/
        if (set_register(args[0], destination) == false)
            throw std::runtime_error("expected register");

        source1 = destination;
        //destination is implicitly taken as the source operand of the operation

        /*---argument 2---*/
        if (set_register(args[1], source2) == false) throw std::runtime_error("expected register");

    } else if (args.size() == 3) {
        /*---argument 1---*/
        if (set_register(args[0], destination) == false)
            throw std::runtime_error("expected register");

        /*---argument 2---*/
        if (set_register(args[1], source1) == false) throw std::runtime_error("expected register");

        /*---argument 3---*/
        if (set_register(args[2], source2) == false) throw std::runtime_error("expected register");

    } else {
        throw std::runtime_error("incorrect number of arguments");
    }
}

int semantic_statements::reg_arith_statement::get_compile_case(
        const symbol_table &st, uint32_t pc, const program_options &options) const {

    using statement_id = asm_lang::reg_arith_statement_id;

    if (destination == source1) {
        switch (id) {
            case statement_id::Add:
            case statement_id::Sub:
            case statement_id::Mult:
            case statement_id::Div:
            case statement_id::Multu:
            case statement_id::Divu:
            case statement_id::Nand:
            case statement_id::Nor:
            case statement_id::Xnor:
            case statement_id::Eql:
            case statement_id::Grt:
            case statement_id::Gre:
            case statement_id::Grtu:
            case statement_id::Greu:
            case statement_id::Lsft:
            case statement_id::Rsft:
            case statement_id::Rsfta:
                return (int) compile_case_t::halfword;

            case statement_id::Neql:
            case statement_id::Or:
            case statement_id::And:
            case statement_id::Xor:
                return (int) compile_case_t::fullword;
        }

        assert(!"unreachable");
    }

    return (int) compile_case_t::fullword;
}

binary_data::memory_alloc_t
semantic_statements::reg_arith_statement::get_size(int comp_case_int) const {
    const auto comp_case = (compile_case_t) comp_case_int;

    switch (comp_case) {
        case compile_case_t::fullword:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};

        case compile_case_t::halfword:
            return {.nbytes = 2, .allignment = binary_data::allignment_t::halfword};
    }

    assert(!"unreachable");
}

std::vector<isa::instruction> semantic_statements::reg_arith_statement::gen_instructions(
        int comp_case_int, const symbol_table &st, uint32_t pc) const {
    const auto comp_case = (compile_case_t) comp_case_int;
    switch (comp_case) {
        case compile_case_t::fullword:
            return gen_fullword_instructions();

        case compile_case_t::halfword:
            return gen_halfword_instructions();
    }

    assert(!"unreachable");
}

class fullword_reg_inst_lut {
    using asm_stmnt_id = asm_lang::reg_arith_statement_id;
    isa::inst_id lut[(uint) asm_stmnt_id::LAST];

public:
    fullword_reg_inst_lut() {
        lut[(int) asm_stmnt_id::Add] = isa::inst_id::Add;
        lut[(int) asm_stmnt_id::Sub] = isa::inst_id::Sub;
        lut[(int) asm_stmnt_id::Mult] = isa::inst_id::Mult;
        lut[(int) asm_stmnt_id::Div] = isa::inst_id::Div;
        lut[(int) asm_stmnt_id::Multu] = isa::inst_id::Multu;
        lut[(int) asm_stmnt_id::Divu] = isa::inst_id::Divu;
        lut[(int) asm_stmnt_id::Eql] = isa::inst_id::Eql;
        lut[(int) asm_stmnt_id::Neql] = isa::inst_id::Neql;
        lut[(int) asm_stmnt_id::Grt] = isa::inst_id::Grt;
        lut[(int) asm_stmnt_id::Grtu] = isa::inst_id::Grtu;
        lut[(int) asm_stmnt_id::Gre] = isa::inst_id::Gre;
        lut[(int) asm_stmnt_id::Greu] = isa::inst_id::Greu;
        lut[(int) asm_stmnt_id::Lsft] = isa::inst_id::Lsft;
        lut[(int) asm_stmnt_id::Rsft] = isa::inst_id::Rsft;
        lut[(int) asm_stmnt_id::Rsfta] = isa::inst_id::Rsfta;
        lut[(int) asm_stmnt_id::Nor] = isa::inst_id::Nor;
        lut[(int) asm_stmnt_id::Nand] = isa::inst_id::Nand;
        lut[(int) asm_stmnt_id::Or] = isa::inst_id::Or;
        lut[(int) asm_stmnt_id::And] = isa::inst_id::And;
        lut[(int) asm_stmnt_id::Xor] = isa::inst_id::Xor;
        lut[(int) asm_stmnt_id::Xnor] = isa::inst_id::Xnor;
    };

    isa::inst_id operator[](asm_stmnt_id id) const {
        assert(id < asm_stmnt_id::LAST);
        return lut[(uint) id];
    };
};

std::vector<isa::instruction>
semantic_statements::reg_arith_statement::gen_fullword_instructions() const {
    const static fullword_reg_inst_lut lut;

    auto reg_inst_id = lut[id];

    return {isa::make_reg_inst(reg_inst_id, destination, source1, source2)};
}

class halfword_reg_inst_lut {
    using asm_stmnt_id = asm_lang::reg_arith_statement_id;

    isa::inst_id lut[(uint) asm_stmnt_id::LAST];

public:
    halfword_reg_inst_lut() {
        lut[(int) asm_stmnt_id::Add] = isa::inst_id::Add_h;
        lut[(int) asm_stmnt_id::Sub] = isa::inst_id::Sub_h;
        lut[(int) asm_stmnt_id::Mult] = isa::inst_id::Mult_h;
        lut[(int) asm_stmnt_id::Div] = isa::inst_id::Div_h;
        lut[(int) asm_stmnt_id::Multu] = isa::inst_id::Multu_h;
        lut[(int) asm_stmnt_id::Divu] = isa::inst_id::Divu_h;
        lut[(int) asm_stmnt_id::Eql] = isa::inst_id::Eql_h;
        lut[(int) asm_stmnt_id::Grt] = isa::inst_id::Grt_h;
        lut[(int) asm_stmnt_id::Grtu] = isa::inst_id::Grtu_h;
        lut[(int) asm_stmnt_id::Gre] = isa::inst_id::Gre_h;
        lut[(int) asm_stmnt_id::Greu] = isa::inst_id::Greu_h;
        lut[(int) asm_stmnt_id::Lsft] = isa::inst_id::Lsft_h;
        lut[(int) asm_stmnt_id::Rsft] = isa::inst_id::Rsft_h;
        lut[(int) asm_stmnt_id::Rsfta] = isa::inst_id::Rsfta_h;
        lut[(int) asm_stmnt_id::Nor] = isa::inst_id::Nor_h;
        lut[(int) asm_stmnt_id::Nand] = isa::inst_id::Nand_h;
        lut[(int) asm_stmnt_id::Xnor] = isa::inst_id::Xnor_h;
        lut[(int) asm_stmnt_id::Or] = isa::inst_id::INVALID;
        lut[(int) asm_stmnt_id::And] = isa::inst_id::INVALID;
        lut[(int) asm_stmnt_id::Xor] = isa::inst_id::INVALID;
        lut[(int) asm_stmnt_id::Neql] = isa::inst_id::INVALID;
    };

    isa::inst_id operator[](asm_stmnt_id id) const {
        assert(id < asm_stmnt_id::LAST);
        return lut[(uint) id];
    };
};

std::vector<isa::instruction>
semantic_statements::reg_arith_statement::gen_halfword_instructions() const {
    static const halfword_reg_inst_lut lut;

    auto inst_id = lut[id];
    assert(inst_id != isa::inst_id::INVALID);

    return {isa::make_half_reg_inst(inst_id, destination, source2)};
}

semantic_statements::immediate_arith_statement::immediate_arith_statement(
        const syntax::inst_statement &inst_stmnt, asm_lang::immediate_arith_statement_id id) {

    this->id = id;
    auto &args = inst_stmnt.args;

    if (args.size() == 2) {
        //---argument 1---//
        if (set_register(args[0], destination) == false)
            throw std::runtime_error("expected register");

        //set destination register as implicit source register
        source = destination;

        //---argument 2---//
        if (set_immediate(args[1], immediate) == false)
            throw std::runtime_error("expected immediate integer");
    } else if (args.size() == 3) {
        //---argument 1---//
        if (set_register(args[0], destination) == false)
            throw std::runtime_error("expected register");

        //---argument 2---//
        if (set_register(args[1], source) == false) throw std::runtime_error("expected register");

        //---argument 3---//
        if (set_immediate(args[2], immediate) == false)
            throw std::runtime_error("expected immediate integer");

    } else {
        throw std::runtime_error("Wrong number of arguments");
    }
}


int semantic_statements::immediate_arith_statement::get_compile_case(
        const symbol_table &st, uint32_t pc, const program_options &options) const {
    switch (id) {
        case asm_lang::immediate_arith_statement_id::Lsfti:
        case asm_lang::immediate_arith_statement_id::Rsfti:
        case asm_lang::immediate_arith_statement_id::Rsftia:
            if (unsigned_bitwidth(immediate) > 5)
                throw std::runtime_error("Immediate integer too large");

            if (destination == source) return (int) compile_case_t::short_shift;

            return (int) compile_case_t::long_shift;

        case asm_lang::immediate_arith_statement_id::Addi:
            if (signed_bitwidth(immediate) <= isa::halfword_immediate_bitsize + 1 &&
                destination == source)
                return (int) compile_case_t::short_add;

            if (signed_bitwidth(immediate) <= isa::imm_immediate_bitsize)
                return (int) compile_case_t::fullword;

            throw std::runtime_error("Immediate integer too large");

        case asm_lang::immediate_arith_statement_id::Multi:
        case asm_lang::immediate_arith_statement_id::Divi:
            if (signed_bitwidth(immediate) > isa::imm_immediate_bitsize)
                throw std::runtime_error("Immediate integer too large");

            return (int) compile_case_t::fullword;

        case asm_lang::immediate_arith_statement_id::Multui:
        case asm_lang::immediate_arith_statement_id::Divui:
        case asm_lang::immediate_arith_statement_id::Andi:
        case asm_lang::immediate_arith_statement_id::Ori:
        case asm_lang::immediate_arith_statement_id::Xori:
            if (unsigned_bitwidth(immediate) > isa::imm_immediate_bitsize)
                throw std::runtime_error("Immediate integer too large");

            return (int) compile_case_t::fullword;
        default:
            assert(!"unreachable");
    }

    return (int) compile_case_t::undetermined;
}

binary_data::memory_alloc_t
semantic_statements::immediate_arith_statement::get_size(int comp_case_int) const {
    auto comp_case = (compile_case_t) comp_case_int;
    switch (comp_case) {
        case compile_case_t::fullword:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::word};

        case compile_case_t::short_add:
        case compile_case_t::short_shift:
            return {.nbytes = 2, .allignment = binary_data::allignment_t::halfword};

        case compile_case_t::long_shift:
            return {.nbytes = 4, .allignment = binary_data::allignment_t::halfword};
    }

    assert(!"unreachable");
}


class imm_stmnt_to_imm_inst {
    using asm_stmnt_id = asm_lang::immediate_arith_statement_id;

    isa::inst_id lut[(uint) asm_stmnt_id::LAST];

public:
    imm_stmnt_to_imm_inst() {
        lut[(int) asm_stmnt_id::Xori] = isa::inst_id::Xori;
        lut[(int) asm_stmnt_id::Ori] = isa::inst_id::Ori;
        lut[(int) asm_stmnt_id::Addi] = isa::inst_id::Addi;
        lut[(int) asm_stmnt_id::Multi] = isa::inst_id::Multi;
        lut[(int) asm_stmnt_id::Divi] = isa::inst_id::Divi;
        lut[(int) asm_stmnt_id::Multui] = isa::inst_id::Multui;
        lut[(int) asm_stmnt_id::Divui] = isa::inst_id::Divui;
        lut[(int) asm_stmnt_id::Lsfti] = isa::inst_id::Lsfti;
        lut[(int) asm_stmnt_id::Rsfti] = isa::inst_id::Rsfti;
        lut[(int) asm_stmnt_id::Rsftia] = isa::inst_id::Rsftia;
    };

    isa::inst_id operator[](asm_stmnt_id id) const {
        assert(id < asm_stmnt_id::LAST);
        return lut[(uint) id];
    };
};

std::vector<isa::instruction> semantic_statements::immediate_arith_statement::gen_instructions(
        int comp_case_int, const symbol_table &st, uint32_t pc) const {
    static const imm_stmnt_to_imm_inst fullword_imm_lut;
    const auto comp_case = (compile_case_t) comp_case_int;
    assert(comp_case != compile_case_t::undetermined);

    std::vector<isa::instruction> instructions;
    switch (comp_case) {
        case compile_case_t::fullword: {
            const auto inst_id = fullword_imm_lut[id];
            return {isa::make_immediate_inst(inst_id, destination, source, immediate)};
        }

        case compile_case_t::long_shift: {
            const auto shift_inst_id = fullword_imm_lut[id];
            return {isa::make_half_reg_inst(isa::inst_id::Mov, destination, source),
                    isa::make_half_imm_inst(shift_inst_id, destination, immediate)};
        }

        case compile_case_t::short_shift: {
            const auto shift_inst_id = fullword_imm_lut[id];
            return {isa::make_half_imm_inst(shift_inst_id, destination, immediate)};
        }

        case compile_case_t::short_add: {
            const isa::inst_id inst_id = (immediate < 0) ? isa::inst_id::Decr : isa::inst_id::Incr;
            return {isa::make_half_imm_inst(inst_id, destination, immediate)};
        }
    }

    assert(!"unreachable");
}

semantic_statements::asm_statement::asm_statement(syntax::statement &&syn_stmnt) {
    switch (syn_stmnt.type) {
        case syntax::statement_type::inst:
            type_m = types::instruction_statement;
            produce_inst_statement(std::move(syn_stmnt.inst), inst_stmnt_ptr_m);
            break;
        case syntax::statement_type::dir:
            type_m = types::directive_statement;
            dir_stmnt_m = directive_statement(std::move(syn_stmnt.dir));
            break;
    }
}

void semantic_statements::asm_statement::produce_inst_statement(
        syntax::inst_statement &&syn_inst_stmnt,
        std::unique_ptr<semantic_statements::inst_statement_i> &stmnt_ptr) {

    asm_lang::inst_statement_info info;
    if (asm_lang::string_to_instruction_info(syn_inst_stmnt.mnemonic, info) == false) assert(false);

    switch (info.type) {
        case asm_lang::inst_statement_type::reg_arith:
            stmnt_ptr = std::move(std::make_unique<semantic_statements::reg_arith_statement>(
                    syn_inst_stmnt, info.reg_arith));
            break;
        case asm_lang::inst_statement_type::imm_arith:
            stmnt_ptr = std::move(std::make_unique<semantic_statements::immediate_arith_statement>(
                    syn_inst_stmnt, info.imm_arith));
            break;
        case asm_lang::inst_statement_type::unary:
            stmnt_ptr = std::move(std::make_unique<semantic_statements::unary_statement>(
                    syn_inst_stmnt, info.unary));
            break;
        case asm_lang::inst_statement_type::set:
            stmnt_ptr = std::move(
                    std::make_unique<semantic_statements::set_statement>(syn_inst_stmnt, info.set));
            break;
        case asm_lang::inst_statement_type::jump:
            stmnt_ptr = std::move(std::make_unique<semantic_statements::jump_statement>(
                    syn_inst_stmnt, info.jump));
            break;
        case asm_lang::inst_statement_type::branch:
            stmnt_ptr = std::move(std::make_unique<semantic_statements::branch_statement>(
                    syn_inst_stmnt, info.branch));
            break;
        case asm_lang::inst_statement_type::data:
            stmnt_ptr = std::move(std::make_unique<semantic_statements::data_statement>(
                    syn_inst_stmnt, info.data));
            break;
        default:
            assert(!"unreachable");
    }

    //---set inst label---//
    if (syn_inst_stmnt.label.size() != 0) stmnt_ptr->set_label(std::move(syn_inst_stmnt.label));
}


semantic_statements::directive_statement::directive_statement(
        const syntax::dir_statement &syn_dir) {
    asm_lang::directive_info dir_info;
    if (asm_lang::string_to_directive_info(syn_dir.directive, dir_info) == false)
        throw std::runtime_error("unknown directive");

    //---set directive by type--//
    type_m = dir_info.type;
    switch (type_m) {
        case asm_lang::directive_type::data:
            //---construct data directive---//
            data_m = data_directive(syn_dir, dir_info.data_id);
            break;

        case asm_lang::directive_type::section:
            //---set section type---//
            switch (dir_info.section_id) {
                case asm_lang::section_directive_id::text:
                    section_m = binary::section_t::text;
                    break;
                case asm_lang::section_directive_id::data:
                    section_m = binary::section_t::data;
                    break;
                case asm_lang::section_directive_id::bss:
                    section_m = binary::section_t::bss;
                    break;
                case asm_lang::section_directive_id::rodata:
                    section_m = binary::section_t::rodata;
                    break;
                default:
                    assert(!"unreachable");
            }
            break;

        case asm_lang::directive_type::symbol:
            //---set symbol directive---//
            symbol_m.id = dir_info.sym_id;
            if (syn_dir.args[0].type != syntax::dir_arg_type::label)
                throw std::runtime_error("expected label argument");

            symbol_m.identifier = syn_dir.args[0].identifier;

            break;
        default:
            assert(!"unreachable");
    }
}

semantic_statements::data_directive::data_directive(const syntax::dir_statement &syn_stmnt,
                                                    asm_lang::data_directive_id id) {
    const auto &args = syn_stmnt.args;

    //---set label--//
    if (syn_stmnt.label.size() != 0) {
        has_label_m = true;
        label_m = {syn_stmnt.label};
    } else {
        has_label_m = false;
    }

    //---check argument types---//
    for (auto &arg: args)
        if (arg.type != syntax::dir_arg_type::integer)
            throw std::runtime_error("expected integer arguments");


    //if no integer arguments given one, at least one implicit zero integer is used

    //---determine allignment---//
    binary_data::allignment_t allignment;
    switch (id) {
        case asm_lang::data_directive_id::word:
        case asm_lang::data_directive_id::word_array:
            allignment = binary_data::allignment_t::word;
            break;
        case asm_lang::data_directive_id::halfword:
        case asm_lang::data_directive_id::halfword_array:
            allignment = binary_data::allignment_t::halfword;
            break;
        case asm_lang::data_directive_id::byte:
        case asm_lang::data_directive_id::byte_array:
            allignment = binary_data::allignment_t::byte;
            break;
        default:
            assert(!"unreachable");
    }

    //---set size and values---//
    //---determine number of values---//
    switch (id) {
        case asm_lang::data_directive_id::byte:
        case asm_lang::data_directive_id::halfword:
        case asm_lang::data_directive_id::word: {
            uint32_t n_values = args.size();
            if (n_values == 0) n_values = 1;
            data.memory_alloc = {.nbytes = word_size(allignment) * n_values,
                                 .allignment = allignment};

            bool is_zero_data = (args.size() == 0);
            for (auto &arg: args) {
                is_zero_data = (is_zero_data && arg.int_val == 0);
                if (arg.type != syntax::dir_arg_type::integer)
                    throw std::runtime_error("expected integer argument");

                data.values.push_back(arg.int_val);
            }

            if (is_zero_data) data.values.clear();

            data.zero_data = is_zero_data;
        } break;
        case asm_lang::data_directive_id::byte_array:
        case asm_lang::data_directive_id::halfword_array:
        case asm_lang::data_directive_id::word_array: {
            if (args.size() != 1) throw std::runtime_error("expected one integer");

            const uint32_t n_values = args[0].int_val;
            data.memory_alloc = {.nbytes = word_size(allignment) * n_values,
                                 .allignment = allignment};
            data.zero_data = true;
        } break;

        default:
            assert(!"unreachable");
    }


    //---set integer values---//

    //if no integer argument given implicit zero is used
}