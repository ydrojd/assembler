//
// Created by djordy on 12/12/22.
//

#ifndef ASSEMBLER_ASM_LANG_H
#define ASSEMBLER_ASM_LANG_H

#include <cctype>
#include <map>
#include <string>

#include "magic_enum/magic_enum.hpp"
#include "templates.h"

namespace asm_lang {

const auto forward_label_prefix = "f_";
const auto backward_label_prefix = "b_";

enum struct inst_statement_type { imm_arith = 0, reg_arith, unary, data, branch, jump, set };

enum struct reg_arith_statement_id {
    Add = 0,
    Sub,
    Mult,
    Div,
    Multu,
    Divu,
    Eql,
    Neql,
    Grt,
    Grtu,
    Gre,
    Greu,
    Lsft,
    Rsft,
    Rsfta,
    Nor,
    Nand,
    Or,
    And,
    Xor,
    Xnor,
    LAST
};

enum struct immediate_arith_statement_id {
    Xori = 0,
    Ori,
    Andi,
    Addi,
    Multi,
    Divi,
    Multui,
    Divui,
    Lsfti,
    Rsfti,
    Rsftia,
    LAST
};

enum struct unary_statement_id {
    Neg,
    Not,
};

enum struct data_statement_id {
    Sw,
    Sh,
    Sb,
    Lw,
    Lh,
    Lb,
    Lhu,
    Lbu,
    LAST,
};
enum struct branch_statement_id {
    Beq,
    Bne,
    Bgr,
    Bgru,
    Bge,
    Bgeu,
    LAST,
};

enum struct jump_statement_id {
    Jal,
    Jmp,
};

enum struct set_statement_id {
    set,
    setupr,
};

enum struct directive_type { section, data, symbol };

enum struct section_directive_id {
    text,
    data,
    bss,
    rodata,
    LAST,
};

enum struct data_directive_id {
    word,
    halfword,
    byte,
    word_array,
    halfword_array,
    byte_array,
    LAST,
};

enum struct symbol_directive_id {
    global,
    extern_ex,
    extern_data,
};

struct directive_info {
    directive_type type;
    union {
        section_directive_id section_id;
        data_directive_id data_id;
        symbol_directive_id sym_id;
    };
};

const static std::unordered_map<std::string_view, directive_info> string_to_directive_map{
        {".text", {.type = directive_type::section, .section_id = section_directive_id::text}},
        {".data", {.type = directive_type::section, .section_id = section_directive_id::data}},
        {".bss", {.type = directive_type::section, .section_id = section_directive_id::bss}},
        {".word", {.type = directive_type::data, .data_id = data_directive_id::word}},
        {".halfword", {.type = directive_type::data, .data_id = data_directive_id::halfword}},
        {".byte", {.type = directive_type::data, .data_id = data_directive_id::byte}},
        {".word_array", {.type = directive_type::data, .data_id = data_directive_id::word_array}},
        {".halfword_array",
         {.type = directive_type::data, .data_id = data_directive_id::halfword_array}},
        {".byte_array", {.type = directive_type::data, .data_id = data_directive_id::byte_array}},
        {".bss", {.type = directive_type::section, .section_id = section_directive_id::bss}},
        {".externdata",
         {.type = directive_type::symbol, .sym_id = symbol_directive_id::extern_data}},
        {".externex", {.type = directive_type::symbol, .sym_id = symbol_directive_id::extern_ex}},
        {".global", {.type = directive_type::symbol, .sym_id = symbol_directive_id::global}},
};

inline bool string_to_directive_info(const std::string &str, directive_info &result) {
    const auto it = string_to_directive_map.find(str);
    if (it == string_to_directive_map.end()) return false;

    result = it->second;
    return true;
}

struct inst_statement_info {
    inst_statement_type type;
    union {
        immediate_arith_statement_id imm_arith;
        reg_arith_statement_id reg_arith;
        unary_statement_id unary;
        data_statement_id data;
        branch_statement_id branch;
        jump_statement_id jump;
        set_statement_id set;
    };
};

const static std::unordered_map<std::string, inst_statement_info> string_to_instruction_map{
        //register arithmetic
        {"add", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Add}},
        {"sub", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Sub}},
        {"mult",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Mult}},
        {"div", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Div}},
        {"multu",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Multu}},
        {"divu",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Divu}},
        {"eql", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Eql}},
        {"neql",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Neql}},
        {"grt", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Grt}},
        {"grtu",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Grtu}},
        {"gre", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Gre}},
        {"greu",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Greu}},
        {"lsft",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Lsft}},
        {"rsft",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Rsft}},
        {"rsfta",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Rsfta}},
        {"nor", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Nor}},
        {"nand",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Nand}},
        {"or", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Or}},
        {"and", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::And}},
        {"xor", {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Xor}},
        {"xnor",
         {.type = inst_statement_type::reg_arith, .reg_arith = reg_arith_statement_id::Xnor}},

        //immediate arithmetic
        {"xori",
         {.type = inst_statement_type::imm_arith, .imm_arith = immediate_arith_statement_id::Xori}},
        {"ori",
         {.type = inst_statement_type::imm_arith, .imm_arith = immediate_arith_statement_id::Ori}},
        {"andi",
         {.type = inst_statement_type::imm_arith, .imm_arith = immediate_arith_statement_id::Andi}},
        {"addi",
         {.type = inst_statement_type::imm_arith, .imm_arith = immediate_arith_statement_id::Addi}},
        {"multi",
         {.type = inst_statement_type::imm_arith,
          .imm_arith = immediate_arith_statement_id::Multi}},
        {"divi",
         {.type = inst_statement_type::imm_arith, .imm_arith = immediate_arith_statement_id::Divi}},
        {"multui",
         {.type = inst_statement_type::imm_arith,
          .imm_arith = immediate_arith_statement_id::Multui}},
        {"divui",
         {.type = inst_statement_type::imm_arith,
          .imm_arith = immediate_arith_statement_id::Divui}},
        {"lsfti",
         {.type = inst_statement_type::imm_arith,
          .imm_arith = immediate_arith_statement_id::Lsfti}},
        {"rsfti",
         {.type = inst_statement_type::imm_arith,
          .imm_arith = immediate_arith_statement_id::Rsfti}},
        {"rsftia",
         {.type = inst_statement_type::imm_arith,
          .imm_arith = immediate_arith_statement_id::Rsftia}},

        //unary
        {"neg", {.type = inst_statement_type::unary, .unary = unary_statement_id::Neg}},
        {"not", {.type = inst_statement_type::unary, .unary = unary_statement_id::Not}},

        //data
        {"sw", {.type = inst_statement_type::data, .data = data_statement_id::Sw}},
        {"sh", {.type = inst_statement_type::data, .data = data_statement_id::Sh}},
        {"sb", {.type = inst_statement_type::data, .data = data_statement_id::Sb}},

        {"lw", {.type = inst_statement_type::data, .data = data_statement_id::Lw}},
        {"lh", {.type = inst_statement_type::data, .data = data_statement_id::Lh}},
        {"lb", {.type = inst_statement_type::data, .data = data_statement_id::Lb}},
        {"lhu", {.type = inst_statement_type::data, .data = data_statement_id::Lhu}},
        {"lbu", {.type = inst_statement_type::data, .data = data_statement_id::Lbu}},

        //branch
        {"beq", {.type = inst_statement_type::branch, .branch = branch_statement_id::Beq}},
        {"bne", {.type = inst_statement_type::branch, .branch = branch_statement_id::Bne}},
        {"bgr", {.type = inst_statement_type::branch, .branch = branch_statement_id::Bgr}},
        {"bgru", {.type = inst_statement_type::branch, .branch = branch_statement_id::Bgru}},
        {"bge", {.type = inst_statement_type::branch, .branch = branch_statement_id::Bge}},
        {"bgeu", {.type = inst_statement_type::branch, .branch = branch_statement_id::Bgeu}},

        //jump
        {"jmp", {.type = inst_statement_type::jump, .jump = jump_statement_id::Jmp}},
        {"jal", {.type = inst_statement_type::jump, .jump = jump_statement_id::Jal}},
        //set
        {"set", {.type = inst_statement_type::set, .set = set_statement_id::set}}};

inline bool string_to_instruction_info(const std::string &str, inst_statement_info &info) {
    const auto it = string_to_instruction_map.find(str);

    if (it == string_to_instruction_map.end()) return false;

    info = it->second;
    return true;
}


}// namespace asm_lang

#endif//ASSEMBLER_ASM_LANG_H
