//
// Created by djordy on 12/11/22.
//

#ifndef ASSEMBLER_SEMANTIC_STATEMENT_H
#define ASSEMBLER_SEMANTIC_STATEMENT_H

#include <inttypes.h>
#include <memory>
#include <stdbool.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "asm_lang.h"
#include "isa.h"
#include "symbol_table.h"
#include "syntax.h"
#include "binary_data.h"
#include "program_options.h"

namespace semantic_statements {

using section_t = asm_lang::section_directive_id;

struct label_t {
    std::string identifier = "";
    int32_t symbol_id = 0;
};

struct label_operand {
    label_t label = {};
    int32_t offset = 0;
};

using reg_t = isa::reg_id;

class inst_statement_i {
    label_t label_m;
    bool has_label_m = false;

public:
    bool has_label() const { return has_label_m; }
    const label_t &get_label() const {
        assert(has_label());
        return label_m;
    }

    label_t &get_label() {
        assert(has_label());
        return label_m;
    }

    void set_label(std::string &&label) {
        label_m = {std::move(label)};
        has_label_m = true;
    }

    virtual int get_compile_case(const symbol_table &st, uint32_t pc,
                                 const program_options &options) const = 0;
    virtual binary_data::memory_alloc_t get_size(int comp_case_int) const = 0;
    virtual std::vector<isa::instruction>
    gen_instructions(int comp_case_int, const symbol_table &st,
                     uint32_t pc) const = 0;
    virtual bool has_label_operand() const = 0;
    virtual label_operand &get_label_operand() = 0;
    const virtual label_operand &get_label_operand() const = 0;
    virtual binary::reloc_type get_reloc_type(int comp_case_int,
                                              const symbol_table &st) const {return binary::reloc_type::none;};
};

static uint unsigned_bitwidth(uint64_t val) { return std::bit_width((uint64_t) val); }

static uint signed_bitwidth(int64_t val) {
    if (val < 0) {
        auto uval = (uint64_t) -val;
        return (std::bit_width(uval - 1) + 1);
    } else {
        return unsigned_bitwidth(val) + 1;
    }
}

static bool set_register(const syntax::inst_arg &arg, reg_t &reg) {
    if (arg.type != syntax::inst_arg_type::reg) { return false; }

    bool found_reg = isa::string_to_reg_id(arg.reg, reg);
    return found_reg;
}

static bool set_immediate(const syntax::inst_arg &arg, int32_t &imm) {
    if (arg.type != syntax::inst_arg_type::integer) return false;

    imm = arg.int_val;
    return true;
}

static bool set_label_operand(const syntax::inst_arg &arg, label_operand &lbl_op) {
    if(arg.type != syntax::inst_arg_type::label)
        return false;

    lbl_op.label.identifier = arg.label;
    return true;
}

static bool set_label_operand_offset(const syntax::inst_arg &arg,
                               label_operand &address) {
    if(arg.type != syntax::inst_arg_type::integer)
        return false;

    address.offset = arg.int_val;
    return true;
}

class reg_arith_statement : public inst_statement_i {
    enum struct compile_case_t {
        undetermined = 0,
        halfword,
        fullword,
    };

    asm_lang::reg_arith_statement_id id;

    reg_t destination;
    reg_t source1;
    reg_t source2;

    std::vector<isa::instruction> gen_fullword_instructions() const;
    std::vector<isa::instruction> gen_halfword_instructions() const;

public:
    reg_arith_statement(const syntax::inst_statement &inst_stmnt, asm_lang::reg_arith_statement_id id);
    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;
    binary_data::memory_alloc_t get_size(int comp_case_int) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case_int,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;
    bool has_label_operand() const override { return false; };
    label_operand &get_label_operand() override { assert(false); };
    const label_operand &get_label_operand() const override { assert(false); };
};

class immediate_arith_statement : public inst_statement_i {
    enum struct compile_case_t {
        undetermined = 0,
        fullword,
        long_shift,
        short_shift,
        short_add,
    };

    asm_lang::immediate_arith_statement_id id;

    reg_t destination;
    reg_t source;
    int32_t immediate;

public:
    immediate_arith_statement(const syntax::inst_statement &inst_stmnt,
                              asm_lang::immediate_arith_statement_id id);
    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;
    binary_data::memory_alloc_t get_size(int comp_case_int) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case_int,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;
    bool has_label_operand() const override { return false; }
    label_operand &get_label_operand() override { assert(false); };
    const label_operand &get_label_operand() const override { assert(false); };

};

class branch_statement : public inst_statement_i {
    enum struct compile_case_t {
        undetermined = 0,
        long_branch,
        short_branch,
    };

    asm_lang::branch_statement_id id;
    reg_t operand1;
    reg_t operand2;
    label_operand jump_label;

public:
    branch_statement(const syntax::inst_statement &inst_stmnt,
                     asm_lang::branch_statement_id id);
    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;
    binary_data::memory_alloc_t get_size(int comp_case_int) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case_int,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;
    bool has_label_operand() const override { return true;};
    label_operand &get_label_operand() override { return jump_label; };
    const label_operand &get_label_operand() const override { return jump_label; };
    virtual binary::reloc_type get_reloc_type() {return binary::reloc_type::dummy;};
};

class jump_statement : public inst_statement_i {
    enum struct compile_case_t {
        undetermined = 0,
        reg_jump,
        short_no_jump,
        short_ra_jump,
        full_jump,
    };

    enum struct destination_types { reg, address };

    destination_types dest_type;
    asm_lang::jump_statement_id id;

    reg_t return_reg;
    reg_t destination_reg;
    label_operand offset;

public:
    jump_statement(const syntax::inst_statement &inst_stmnt,
                   asm_lang::jump_statement_id id);

    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;

    binary_data::memory_alloc_t get_size(int comp_case_int) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case_int,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;
    bool has_label_operand() const override {
        return (dest_type == destination_types::address);
    };

    label_operand &get_label_operand() override {
        assert(dest_type == destination_types::address);
        return offset;
    };

    const label_operand &get_label_operand() const override {
        assert(dest_type == destination_types::address);
        return offset;
    };

    binary::reloc_type get_reloc_type(int comp_case_int,
                                      const symbol_table &st) const override;
};

class unary_statement : public inst_statement_i {
    enum struct compile_case_t {
        undetermined = 0,
        half_reg,
        full_reg,
    };

    asm_lang::unary_statement_id id;

    reg_t destination;
    reg_t operand;

public:
    unary_statement(const syntax::inst_statement &inst_stmnt,
                    asm_lang::unary_statement_id id);
    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;
    binary_data::memory_alloc_t get_size(int comp_case_int) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case_int,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;
    bool has_label_operand() const override { return false; };
    label_operand &get_label_operand() override { assert(false); };
    const label_operand &get_label_operand() const override { assert(false); };
};

class set_statement : public inst_statement_i {
    enum struct source_types {
        address_label,
        integer,
        reg,
    };

    enum struct compile_case_t {
        undetermined = 0,
        reg_mov,
        pc_rel,
        data_label,
        int_lower_fit,
        int_upper_fit,
        int_full
    };

    asm_lang::set_statement_id id;
    source_types src_type;
    reg_t destination_reg;

    int32_t source_integer;
    reg_t source_reg;
    label_operand source_address;

    int32_t get_source_immediate() const;

public:
    set_statement(const syntax::inst_statement &inst_stmnt, asm_lang::set_statement_id);
    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;
    binary_data::memory_alloc_t get_size(int comp_case_int) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case_int,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;
    bool has_label_operand() const override {
        return (src_type == source_types::address_label);
    };

    label_operand &get_label_operand() override { return source_address; };
    const label_operand &get_label_operand() const override { return source_address; };
    binary::reloc_type get_reloc_type(int comp_case_int,
                                      const symbol_table &st) const override;
};

class data_statement : public inst_statement_i {
    enum struct compile_case_t {
        undetermined = 0,
        short_reg,
        long_reg,
        label,
    };

    reg_t operand1;
    reg_t reg_location;
    int32_t reg_location_offset;

    bool has_label_operand_m = false;
    label_operand label_location;
    asm_lang::data_statement_id id;

public:
    data_statement(const syntax::inst_statement &inst_stmnt,
                    asm_lang::data_statement_id id);

    int get_compile_case(const symbol_table &st, uint32_t pc,
                         const program_options &options) const override;
    binary_data::memory_alloc_t get_size(int comp_case) const override;
    std::vector<isa::instruction> gen_instructions(int comp_case,
                                                   const symbol_table &st,
                                                   uint32_t pc) const override;

    bool has_label_operand() const override { return has_label_operand_m;};
    label_operand &get_label_operand() override {
        assert(has_label_operand_m);
        return label_location;};

    const label_operand &get_label_operand() const override {
        assert(has_label_operand_m);
        return label_location;};

    binary::reloc_type get_reloc_type(int comp_case_int, const symbol_table &st) const override;
};

class data_directive {
    binary_data::data_alloc_t data;

    label_t label_m;
    bool has_label_m;

public:
    data_directive() = default;
    data_directive(const syntax::dir_statement &syn_stmnt,
                   asm_lang::data_directive_id id);
    binary_data::memory_alloc_t get_size() const { return data.memory_alloc; }

    bool has_label() const { return has_label_m; }

    const label_t &get_label() const { return label_m; }

    label_t &get_label() { return label_m; }

    binary_data::data_alloc_t &get_data() {
        return data;
    }

    const binary_data::data_alloc_t &get_data() const {
        return data;
    }
};

struct symbol_directive {
    asm_lang::symbol_directive_id id;
    std::string identifier;
};

class directive_statement {
private:
    asm_lang::directive_type type_m;
    data_directive data_m;
    binary::section_t section_m;
    symbol_directive symbol_m;

public:
    directive_statement(const syntax::dir_statement &syn_dir);
    directive_statement() = default;

    data_directive &get_data() {
        assert(type_m == asm_lang::directive_type::data);
        return data_m;
    }

    symbol_directive &get_symbol() {
        assert(type_m == asm_lang::directive_type::symbol);
        return symbol_m;
    }

    binary::section_t get_section() const {
        assert(type_m == asm_lang::directive_type::section);
        return section_m;
    }

    asm_lang::directive_type get_type() const { return type_m; }

};

class asm_statement {
public:
    enum struct types {
        instruction_statement,
        directive_statement,
    };

private:
    types type_m;
    std::unique_ptr<inst_statement_i> inst_stmnt_ptr_m;
    directive_statement dir_stmnt_m;

    static void
    produce_inst_statement(syntax::inst_statement &&syn_inst_stmnt,
                           std::unique_ptr<semantic_statements::inst_statement_i> &stmnt_ptr);

public:
    asm_statement(syntax::statement &&syn_stmnt);
    types get_type() const { return type_m; }
    std::unique_ptr<inst_statement_i> &get_inst_stmnt() {
        assert(type_m == types::instruction_statement);
        return inst_stmnt_ptr_m;
    }
    const std::unique_ptr<inst_statement_i> &get_inst_stmnt() const {
        assert(type_m == types::instruction_statement);
        return inst_stmnt_ptr_m;
    }

    directive_statement &get_dir_stmnt() {
        assert(type_m == types::directive_statement);
        return dir_stmnt_m;
    }

    const directive_statement &get_dir_stmnt() const {
        assert(type_m == types::directive_statement);
        return dir_stmnt_m;
    }

    const bool is_instruction() const { return type_m == types::instruction_statement;}
};

}// namespace semantic_statements


#endif//ASSEMBLER_SEMANTIC_STATEMENT_H
