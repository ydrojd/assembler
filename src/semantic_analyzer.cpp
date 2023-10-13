//
// Created by djordy on 1/2/23.
//
#include <unordered_set>

#include "semantic_statement.h"
#include "binary.h"
#include "program_options.h"
#include "semantic_analyzer.h"
#include "syntax.h"

static void insert_data_statement(semantic_statements::directive_statement &stmnt,
                                  binary::section_t working_section,
                                  std::vector<semantic_statements::data_directive> &rodata,
                                  std::vector<semantic_statements::data_directive> &data,
                                  std::vector<semantic_statements::data_directive> &bss);

static void
link_backward_labels(std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_statements);
static void
link_forward_labels(std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_statements);
static const std::vector<int>
get_compile_cases(symbol_table &st,
                  std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_stmnts,
                  std::unordered_set<std::string> &globals, const program_options &options);

std::vector<isa::instruction>
generate_instructions(std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_stmnts,
                      const std::vector<int> &compile_cases, symbol_table &st);

static void calculate_text_label_addresses(
        std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_stmnts,
        const std::vector<int> &compile_cases, symbol_table &st);

std::vector<binary_data::data_alloc_t>
process_data_directives(binary::section_t section,
                        const std::vector<semantic_statements::data_directive> &data_stmnts, symbol_table &st,
                        std::unordered_set<std::string> &globals);

struct assembly_statements {
    std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> text;
    std::vector<semantic_statements::data_directive> rodata;
    std::vector<semantic_statements::data_directive> data;
    std::vector<semantic_statements::data_directive> bss;
};

assembly_statements generate_asm_statements(syntax &parser,
                                            std::unordered_set<std::string> &globals);


compilation_unit semantic_analyzer(syntax &parser, program_options &options) {
    compilation_unit comp_unit;

    //---parsing assembly statements---//
    std::vector<semantic_statements::symbol_directive> symbol_dirs;
    std::unordered_set<std::string> globals;
    auto statements = generate_asm_statements(parser, globals);

    //---process data sections---//
    comp_unit.data = process_data_directives(binary::section_t::data, statements.data, comp_unit.st,
                                             globals);
    comp_unit.rodata = process_data_directives(binary::section_t::rodata, statements.rodata,
                                               comp_unit.st, globals);
    comp_unit.bss =
            process_data_directives(binary::section_t::bss, statements.bss, comp_unit.st, globals);

    //---process text section---//
    //---link backward/forward text labels---//
    link_backward_labels(statements.text);
    link_forward_labels(statements.text);

    //---get text compile cases and insert symbols---//
    const auto compile_cases = get_compile_cases(comp_unit.st, statements.text, globals, options);

    //---text label calculation---//
    calculate_text_label_addresses(statements.text, compile_cases, comp_unit.st);

    //---generate instructions and relocs---//
    comp_unit.instructions = generate_instructions(statements.text, compile_cases, comp_unit.st);

    return comp_unit;
}

assembly_statements generate_asm_statements(syntax &parser,
                                            std::unordered_set<std::string> &globals) {


    using stmnt_types = semantic_statements::asm_statement::types;

    struct assembly_statements statements;
    binary::section_t working_section = binary::section_t::text;

    for (auto ret = parser.parse_statement(); ret.second != true; ret = parser.parse_statement()) {
	auto &&syntax_statement = ret.first;
        semantic_statements::asm_statement asm_stmnt(std::move(syntax_statement));
        switch (asm_stmnt.get_type()) {
            case stmnt_types::instruction_statement: {
                if (working_section != binary::section_t::text)
                    throw std::runtime_error("instruction statement outside text section");

                statements.text.push_back(std::move(asm_stmnt.get_inst_stmnt()));
            } break;

            case stmnt_types::directive_statement: {
                auto &dir_stmnt = asm_stmnt.get_dir_stmnt();
                switch (dir_stmnt.get_type()) {
                    case asm_lang::directive_type::section:
                        working_section = dir_stmnt.get_section();
                        break;

                    case asm_lang::directive_type::data:
                        insert_data_statement(dir_stmnt, working_section, statements.rodata,
                                              statements.data, statements.bss);
                        break;

                    case asm_lang::directive_type::symbol:
                        globals.insert(dir_stmnt.get_symbol().identifier);
                        break;
                }
            } break;

            default:
                assert(!"unreachable");
        }
    }

    return statements;
}

static void insert_data_statement(semantic_statements::directive_statement &stmnt,
                                  binary::section_t working_section,
                                  std::vector<semantic_statements::data_directive> &rodata,
                                  std::vector<semantic_statements::data_directive> &data,
                                  std::vector<semantic_statements::data_directive> &bss) {
    switch (working_section) {
        case binary::section_t::data:
            data.push_back(stmnt.get_data());
            break;
        case binary::section_t::rodata:
            rodata.push_back(stmnt.get_data());
            break;
        case binary::section_t::bss:
            bss.push_back(stmnt.get_data());
            break;
        case binary::section_t::text:
            throw std::runtime_error("data statement outside data section");
            break;
        default:
            assert(!"unreachable");
    }
}

static bool is_backward(const semantic_statements::label_t &lbl) {
    return (lbl.identifier.starts_with(asm_lang::backward_label_prefix));
}

static bool is_forward(const semantic_statements::label_t &lbl) {
    return (lbl.identifier.starts_with(asm_lang::forward_label_prefix));
}

static void
link_backward_labels(std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_statements) {

    std::unordered_map<std::string, uint> backward_stmnt_labels;
    for (auto &stmnt: text_statements) {
        //---insert backward---//
        if (stmnt->has_label()) {
            auto &lbl = stmnt->get_label();
            if (is_backward(lbl)) {
                const auto search_it = backward_stmnt_labels.find(lbl.identifier);
                int tag = 0;

                if (search_it == backward_stmnt_labels.end())
                    backward_stmnt_labels.insert({lbl.identifier, 0});
                else
                    tag = (++search_it->second);

                lbl.identifier += "#" + std::to_string(tag);
            }
        }

        //---link backward--//
        if (stmnt->has_label_operand()) {
            auto &lbl = stmnt->get_label_operand().label;
            if (is_backward(lbl)) {
                const auto search_it = backward_stmnt_labels.find(lbl.identifier);
                if (search_it == backward_stmnt_labels.end())
                    throw std::runtime_error("backward label not found: " + lbl.identifier);

                //---set label tag---//
                const auto tag = search_it->second;
                lbl.identifier += "#" + std::to_string(tag);
            }
        }
    }
}

static void
link_forward_labels(std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_statements) {
    std::unordered_map<std::string, uint> forward_stmnt_labels;
    for (int i = text_statements.size() - 1; i >= 0; --i) {//iterate backwards over statements
        auto &stmnt = text_statements[i];

        //---insert forward---//
        if (stmnt->has_label()) {
            auto &lbl = stmnt->get_label();
            if (is_forward(lbl)) {
                auto search_it = forward_stmnt_labels.find(lbl.identifier);

                int tag = 0;
                if (search_it == forward_stmnt_labels.end())
                    forward_stmnt_labels.insert({lbl.identifier, 0});
                else
                    tag = (++search_it->second);

                lbl.identifier += ("#" + std::to_string(tag));
            }
        }

        //---link forward--//
        if (stmnt->has_label_operand()) {
            auto &lbl = stmnt->get_label_operand().label;
            if (is_forward(lbl)) {
                const auto search_it = forward_stmnt_labels.find(lbl.identifier);
                if (search_it == forward_stmnt_labels.end())
                    throw std::runtime_error("forward label not found: " + lbl.identifier);

                //---set label tag---//
                const auto tag = search_it->second;
                lbl.identifier += ("#" + std::to_string(tag));
            }
        }
    }
}

static const std::vector<int>
get_compile_cases(symbol_table &st,
                  std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_stmnts,
                  std::unordered_set<std::string> &globals, const program_options &options) {

    binary_data::alligned_counter inst_address_counter;
    std::vector<binary_data::memory_alloc_t> sizes;

    //---size and label aproximation---//
    for (auto &inst_stmnt: text_stmnts) {
        //---get worst case instruction statement size---//
        const auto compile_case = inst_stmnt->get_compile_case(st, 0, options);
        //jump and branch instructions will return an undetermined compile case, the pc doesn't matter and as such can be set to 0

        //---update address_counter---//
        const auto size = inst_stmnt->get_size(compile_case);
        const auto placement_address = inst_address_counter.increment(size);
        sizes.push_back(size);

        //--insert label in symbol table---//
        if (inst_stmnt->has_label()) {
            const auto &identifier = inst_stmnt->get_label().identifier;
            const bool is_global = globals.find(identifier) != globals.end();
            const symbol sym = {.section = binary::section_t::text,
                                .identifier = identifier,
                                .address = placement_address,
                                .type = symbol_type::fun,
                                .scope = is_global ? symbol_scope::global : symbol_scope::local,
                                .size = size.nbytes};

            if (is_global) globals.erase(identifier);//remove from undefined globals set

            bool insert_success = false;
            const auto symbol_id = st.insert_symbol(std::move(sym), insert_success);
            if (insert_success == false) throw std::runtime_error("duplicate label");

            //---set symbol id in assembly statement---//
            inst_stmnt->get_label().symbol_id = symbol_id;
        }
    }

    //every identifier that is still in the global set must now be an external symbol outside of the translation unit
    //must insert these into to symbol table before we can determine the compile cases
    for (auto &identifier: globals) {
        const symbol sym = {.section = binary::section_t::undefined,
                            .identifier = identifier,
                            .address = 0,
                            .type = symbol_type::undefined,
                            .scope = symbol_scope::external,
                            .size = 0};

        bool insert_succes = false;
        st.insert_symbol(sym, insert_succes);
        if (insert_succes == false) throw std::runtime_error("duplicate global symbol");
    }

    //---determine compile case with approximated text labels---//
    std::vector<int> comp_cases;
    inst_address_counter.reset();
    auto size_it = sizes.begin();
    for (auto &inst_stmnt: text_stmnts) {
        const auto placement_address = inst_address_counter.increment(*size_it);
        if (inst_stmnt->has_label_operand()) {
            auto &operand_label = inst_stmnt->get_label_operand().label;

            //---label operand symbol link---//
            //must set the symbol id of the label operand to determine compile case
            bool found_symbol = false;
            operand_label.symbol_id = st.get_id(operand_label.identifier, found_symbol);
            if (found_symbol == false) throw std::runtime_error("label not found");
        }

        /*---get final compile case---*/
        const auto final_compile_case =
                inst_stmnt->get_compile_case(st, placement_address, options);
        comp_cases.push_back(final_compile_case);
    }

    return comp_cases;
}

static void calculate_text_label_addresses(
        std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_stmnts,
        const std::vector<int> &compile_cases, symbol_table &st) {

    binary_data::alligned_counter inst_address_counter;
    auto compile_case_it = compile_cases.begin();

    for (auto &inst_stmnt: text_stmnts) {
        /*---update address counter---*/
        assert(compile_case_it != compile_cases.end());
        assert(*compile_case_it != 0);
        const auto size = inst_stmnt->get_size(*compile_case_it);
        const auto placement_address = inst_address_counter.increment(size);
        ++compile_case_it;

        /*---update label address---*/
        if (inst_stmnt->has_label()) {
            const auto symbol_id = inst_stmnt->get_label().symbol_id;
            st[symbol_id].address = placement_address;
            st[symbol_id].size = size.nbytes;
        }
    }
}

std::vector<isa::instruction>
generate_instructions(std::vector<std::unique_ptr<semantic_statements::inst_statement_i>> &text_stmnts,
                      const std::vector<int> &compile_cases, symbol_table &st) {

    std::vector<isa::instruction> instructions;
    binary_data::alligned_counter inst_counter;
    auto compile_case_it = compile_cases.begin();

    for (auto &inst_stmnt: text_stmnts) {
        //---update address counter---//
        assert(*compile_case_it != 0);
        assert(compile_case_it != compile_cases.end());
        const auto size = inst_stmnt->get_size(*compile_case_it);
        const auto placement_address = inst_counter.increment(size);

        //---generate instruction---//
        const std::vector<isa::instruction> generated_instructions =
                inst_stmnt->gen_instructions(*compile_case_it, st, placement_address);

        instructions.insert(instructions.end(), generated_instructions.begin(),
                            generated_instructions.end());

        //---insert symbol reference---//
        if (inst_stmnt->has_label_operand()) {
            st.insert_ref({.symbol_id = inst_stmnt->get_label_operand().label.symbol_id,
                           .address = placement_address,
                           .type = inst_stmnt->get_reloc_type(*compile_case_it, st)});
        }

        ++compile_case_it;
    }

    return instructions;
}

std::vector<binary_data::data_alloc_t>
process_data_directives(binary::section_t section,
                        const std::vector<semantic_statements::data_directive> &data_stmnts, symbol_table &st,
                        std::unordered_set<std::string> &globals) {

    std::vector<binary_data::data_alloc_t> data;
    binary_data::alligned_counter data_counter;

    for (auto &data_stmnt: data_stmnts) {
        //---update address counter---//
        const auto placement_address = data_counter.increment(data_stmnt.get_size());

        //---insert label---//
        if (data_stmnt.has_label()) {
            const auto identifier = data_stmnt.get_label().identifier;
            const bool is_global = globals.find(identifier) != globals.end();
            const symbol sym = {.section = section,
                                .identifier = identifier,
                                .address = placement_address,
                                .type = symbol_type::data,
                                .scope = is_global ? symbol_scope::global : symbol_scope::local,
                                .size = data_stmnt.get_size().nbytes};

            if (is_global) globals.erase(identifier);

            bool insert_success = false;
            st.insert_symbol(sym, insert_success);
            if (insert_success == false) throw std::runtime_error("duplicate label");
        }

        //---push back data alloc--//
        data.push_back(data_stmnt.get_data());
    }

    return data;
}
