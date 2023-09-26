//
// Created by djordy on 12/10/22.
//

#ifndef ASSEMBLER_ISA_H
#define ASSEMBLER_ISA_H

#include "magic_enum/magic_enum.hpp"
#include "templates.h"
#include <cctype>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace isa {

const uint opcode_pos = 1;
const uint bitmode_pos = 0;
const uint fun_pos = 6;
const uint sr2_pos = 17;
const uint sr1_pos = 22;
const uint dr_pos = 27;
const uint branch_upperimmediate_pos = 27;
const uint set_immediate_pos = 6;
const uint jump_immediate_pos = 6;
const uint imm_immediate_pos = 8;
const uint halfword_immediate_pos = 6;
const uint halfword_dr_pos = 11;
const uint halfword_sr_pos = 6;
const uint branch_lowerimmediate_pos = 8;

const uint reg_bitsize = 5;
const uint opcode_bitsize = 5;
const uint shortfun_bitsize = 2;
const uint longfun_bitsize = 6;

const uint bitmode_bitsize = 1;
const uint jump_immediate_bitsize = 26;
const uint set_immediate_bitsize = 21;
const uint imm_immediate_bitsize = 14;
const uint branch_upperimmediate_bitsize = 5;
const uint branch_lowerimmediate_bitsize = 9;
const uint halfword_immediate_bitsize = 5;

enum struct instruction_size_type {
    halfword,
    fullword
};

struct encoded_instruction {
    instruction_size_type size;
    union {
        uint32_t fullword;
        uint16_t halfword;
    };
};

enum struct reg_id {
    zero = 0,
    ra = 1,
    sp = 2,
    gp = 3,
    k0 = 4,
    k1 = 5,
    pg = 6,
    ar = 7,
    s0 = 8,
    s1 = 9,
    s2 = 10,
    s3 = 11,
    s4 = 12,
    s5 = 13,
    s6 = 14,
    s7 = 15,
    t0 = 16,
    t1 = 17,
    t2 = 18,
    t3 = 19,
    t4 = 20,
    t5 = 21,
    t6 = 22,
    t7 = 23,
    fn0 = 24,
    fn1 = 25,
    fn2 = 26,
    fn3 = 27,
    fn4 = 28,
    fn5 = 29,
    fn6 = 30,
    fn7 = 31,
    LAST

};

static const auto reg_id_to_str_lut = enum_lut<reg_id, std::string_view>(
        {
                {reg_id::zero, "zero"},
                {reg_id::ra, "ra"},
                {reg_id::sp, "sp"},
                {reg_id::gp, "gp"},
                {reg_id::k0, "k0"},
                {reg_id::k1, "k1"},
                {reg_id::pg, "pg"},
                {reg_id::ar, "ar"},
                {reg_id::s0, "s0"},
                {reg_id::s1, "s1"},
                {reg_id::s2, "s2"},
                {reg_id::s3, "s3"},
                {reg_id::s4, "s4"},
                {reg_id::s5, "s5"},
                {reg_id::s6, "s6"},
                {reg_id::s7, "s7"},
                {reg_id::t0, "t0"},
                {reg_id::t1, "t1"},
                {reg_id::t2, "t2"},
                {reg_id::t3, "t3"},
                {reg_id::t4, "t4"},
                {reg_id::t5, "t5"},
                {reg_id::t6, "t6"},
                {reg_id::t7, "t7"},
                {reg_id::fn0, "fn0"},
                {reg_id::fn1, "fn1"},
                {reg_id::fn2, "fn2"},
                {reg_id::fn3, "fn3"},
                {reg_id::fn4, "fn4"},
                {reg_id::fn5, "fn5"},
                {reg_id::fn6, "fn6"},
                {reg_id::fn7, "fn7"},
        });

enum struct inst_id {
    //register instructions
    Add = 0,
    Sub,
    Mult,
    Div,
    Multu,
    Divu,
    Eql,
    Neql,
    Grt,
    Gre,
    Grtu,
    Greu,
    Lsft,
    Rsft,
    Rsfta,
    Or,
    And,
    Xor,
    Nor,
    Nand,
    Xnor,

    //branch instruction
    Sb,
    Sh,
    Sw,
    Beq,
    Bne,
    Bgr,
    Bgru,
    Bge,
    Bgeu,

    //immediate instruction
    Lb,
    Lh,
    Lw,
    Lbu,
    Lhu,
    Xori,
    Ori,
    Andi,
    Addi,
    Multi,
    Divi,
    Multui,
    Divui,
    Jalr,

    //set instruction
    Sli,
    Sui,
    Apci,

    //jump instruction
    Rji,
    Rjali,

    //halfword register instruction
    Add_h,
    Sub_h,
    Mult_h,
    Div_h,
    Multu_h,
    Divu_h,
    Nand_h,
    Nor_h,
    Xnor_h,
    Eql_h,
    Grt_h,
    Gre_h,
    Grtu_h,
    Greu_h,
    Lsft_h,
    Rsft_h,
    Rsfta_h,
    Jalr_h,
    Mov,

    //halfword immediate instruction
    Lsfti,
    Rsfti,
    Rsftia,
    Incr,
    Decr,

    LAST,
    INVALID,
};

enum struct format_id {
    reg,
    branch,
    immediate,
    set,
    jump,
    half_reg,
    half_immediate,
    LAST,
    INVALID,
};

struct format_operand_form {
    bool dr;
    bool sr1;
    bool sr2;
    bool imm;
};

static const enum_lut<isa::format_id, format_operand_form> format_operand_form_lut({
        {isa::format_id::reg, {.dr = true, .sr1 = true, .sr2 = true, .imm = false}},
        {isa::format_id::branch, {.dr = false, .sr1 = true, .sr2 = true, .imm = true}},
        {isa::format_id::immediate, {.dr = true, .sr1 = true, .sr2 = false, .imm = true}},
        {isa::format_id::set, {.dr = true, .sr1 = false, .sr2 = false, .imm = true}},
        {isa::format_id::jump, {.dr = false, .sr1 = false, .sr2 = false, .imm = true}},
        {isa::format_id::half_reg, {.dr = true, .sr1 = false, .sr2 = true, .imm = false}},
        {isa::format_id::half_immediate, {.dr = true, .sr1 = false, .sr2 = false, .imm = true}},
});

enum struct extension_type {
    one,
    zero,
    sign,
    na,
};

struct inst_type {
    format_id format;
    uint8_t opcode;
    uint8_t funcode;
    bool is_halfword;
    extension_type extension = extension_type::na;
};

static const enum_lut<inst_id, inst_type> inst_type_lut({
        //branch instruction
        {inst_id::Sb, {format_id::branch, 0x0, 0x0, false, extension_type::sign}},
        {inst_id::Sh, {format_id::branch, 0x0, 0x1, false, extension_type::sign}},
        {inst_id::Sw, {format_id::branch, 0x0, 0x2, false, extension_type::sign}},
        {inst_id::Beq, {format_id::branch, 0x1, 0x0, false, extension_type::sign}},
        {inst_id::Bne, {format_id::branch, 0x1, 0x1, false, extension_type::sign}},
        {inst_id::Bgr, {format_id::branch, 0x1, 0x0, false, extension_type::sign}},
        {inst_id::Bgru, {format_id::branch, 0x2, 0x1, false, extension_type::sign}},
        {inst_id::Bge, {format_id::branch, 0x2, 0x2, false, extension_type::sign}},
        {inst_id::Bgeu, {format_id::branch, 0x2, 0x3, false, extension_type::sign}},

        //immediate instruction
        {inst_id::Lb, {format_id::immediate, 0x3, 0x0, false, extension_type::sign}},
        {inst_id::Lh, {format_id::immediate, 0x3, 0x1, false, extension_type::sign}},
        {inst_id::Lw, {format_id::immediate, 0x3, 0x2, false, extension_type::sign}},
        {inst_id::Lbu, {format_id::immediate, 0x4, 0x0, false, extension_type::sign}},
        {inst_id::Lhu, {format_id::immediate, 0x4, 0x1, false, extension_type::sign}},
        {inst_id::Xori, {format_id::immediate, 0x5, 0x0, false, extension_type::zero}},
        {inst_id::Ori, {format_id::immediate, 0x5, 0x1, false, extension_type::zero}},
        {inst_id::Andi, {format_id::immediate, 0x5, 0x2, false, extension_type::one}},
        {inst_id::Addi, {format_id::immediate, 0x6, 0x0, false, extension_type::sign}},
        {inst_id::Jalr, {format_id::immediate, 0x6, 0x1, false, extension_type::sign}},
        {inst_id::Multi, {format_id::immediate, 0x7, 0x0, false, extension_type::sign}},
        {inst_id::Divi, {format_id::immediate, 0x7, 0x1, false, extension_type::sign}},
        {inst_id::Multui, {format_id::immediate, 0x7, 0x2, false, extension_type::zero}},
        {inst_id::Divui, {format_id::immediate, 0x7, 0x3, false, extension_type::zero}},

        //set instruction
        {inst_id::Sli, {format_id::set, 0x8, 0x0, false, extension_type::sign}},
        {inst_id::Sui, {format_id::set, 0x9, 0x0, false, extension_type::na}},
        {inst_id::Apci, {format_id::set, 0xA, 0x0, false, extension_type::na}},

        //register instructions
        {inst_id::Add, {format_id::reg, 0xB, 0x0, false}},
        {inst_id::Sub, {format_id::reg, 0xB, 0x1, false}},
        {inst_id::Mult, {format_id::reg, 0xB, 0x2, false}},
        {inst_id::Div, {format_id::reg, 0xB, 0x3, false}},
        {inst_id::Multu, {format_id::reg, 0xB, 0x4, false}},
        {inst_id::Divu, {format_id::reg, 0xB, 0x5, false}},
        {inst_id::Eql, {format_id::reg, 0xB, 0x6, false}},
        {inst_id::Neql, {format_id::reg, 0xB, 0x7, false}},
        {inst_id::Grt, {format_id::reg, 0xB, 0x8, false}},
        {inst_id::Gre, {format_id::reg, 0xB, 0x9, false}},
        {inst_id::Grtu, {format_id::reg, 0xB, 0xA, false}},
        {inst_id::Greu, {format_id::reg, 0xB, 0xB, false}},
        {inst_id::Lsft, {format_id::reg, 0xB, 0xC, false}},
        {inst_id::Rsft, {format_id::reg, 0xB, 0xD, false}},
        {inst_id::Rsfta, {format_id::reg, 0xB, 0xE, false}},
        {inst_id::Or, {format_id::reg, 0xB, 0xF, false}},
        {inst_id::And, {format_id::reg, 0xB, 0x10, false}},
        {inst_id::Xor, {format_id::reg, 0xB, 0x11, false}},
        {inst_id::Nor, {format_id::reg, 0xB, 0x12, false}},
        {inst_id::Nand, {format_id::reg, 0xB, 0x13, false}},
        {inst_id::Xnor, {format_id::reg, 0xB, 0x14, false}},

        //jump instruction
        {inst_id::Rji, {format_id::jump, 0xC, 0x0, false, extension_type::sign}},
        {inst_id::Rjali, {format_id::jump, 0xD, 0x0, false, extension_type::sign}},

        //Halfword register
        {inst_id::Add_h, {format_id::half_reg, 0x0, 0x0, true}},
        {inst_id::Sub_h, {format_id::half_reg, 0x1, 0x0, true}},
        {inst_id::Mult_h, {format_id::half_reg, 0x2, 0x0, true}},
        {inst_id::Div_h, {format_id::half_reg, 0x3, 0x0, true}},
        {inst_id::Multu_h, {format_id::half_reg, 0x4, 0x0, true}},
        {inst_id::Divu_h, {format_id::half_reg, 0x5, 0x0, true}},
        {inst_id::Nand_h, {format_id::half_reg, 0x6, 0x0, true}},
        {inst_id::Nor_h, {format_id::half_reg, 0x7, 0x0, true}},
        {inst_id::Xnor_h, {format_id::half_reg, 0x8, 0x0, true}},
        {inst_id::Eql_h, {format_id::half_reg, 0x9, 0x0, true}},
        {inst_id::Grt_h, {format_id::half_reg, 0xA, 0x0, true}},
        {inst_id::Gre_h, {format_id::half_reg, 0xB, 0x0, true}},
        {inst_id::Grtu_h, {format_id::half_reg, 0xC, 0x0, true}},
        {inst_id::Greu_h, {format_id::half_reg, 0xD, 0x0, true}},
        {inst_id::Lsft_h, {format_id::half_reg, 0xE, 0x0, true}},
        {inst_id::Rsft_h, {format_id::half_reg, 0xF, 0x0, true}},
        {inst_id::Rsfta_h, {format_id::half_reg, 0x10, 0x0, true}},
        {inst_id::Jalr_h, {format_id::half_reg, 0x11, 0x0, true}},
        {inst_id::Mov, {format_id::half_reg, 0x13, 0x0, true}},

        //Halfword immediate
        {inst_id::Lsfti, {format_id::half_immediate, 0x14, 0x0, true, extension_type::zero}},
        {inst_id::Rsfti, {format_id::half_immediate, 0x15, 0x0, true, extension_type::zero}},
        {inst_id::Rsftia, {format_id::half_immediate, 0x16, 0x0, true, extension_type::zero}},
        {inst_id::Incr, {format_id::half_immediate, 0x17, 0x0, true, extension_type::zero}},
        {inst_id::Decr, {format_id::half_immediate, 0x18, 0x0, true, extension_type::one}},
});

inst_type get_inst_type(inst_id id);

format_id get_format_from_encoding(uint8_t opcode, instruction_size_type size);

inst_id get_inst_id_from_encoding(uint8_t opcode, uint8_t funcode, instruction_size_type size);

class inst_id_to_string_lut_t {
    std::array<std::string_view, (uint) inst_id::LAST> lut;

public:
    inst_id_to_string_lut_t() {
        auto pairs = magic_enum::enum_entries<inst_id>();
        for (auto &p: pairs) {
            if (p.first == inst_id::LAST)
                continue;

            lut[(uint) p.first] = p.second;
        }
    }

    const std::string_view &operator[](inst_id index) const {
        assert(index < inst_id::LAST);
        return lut[(uint) index];
    }
};

static inst_id_to_string_lut_t inst_id_to_string_lut;

struct instruction {
    inst_id id = inst_id::INVALID;
    format_id format;

    reg_id dr;
    reg_id sr1;
    reg_id sr2;
    int32_t immediate;

    encoded_instruction encode() const;

    instruction() = default;

    instruction(encoded_instruction encoded);

    std::string to_str() const;

private:
    void encode_fullword_immediate(uint32_t &inst) const;
    void decode_fullword_immediate(uint32_t encoded_inst);
    void decode_fullword_instruction(uint32_t encoded_inst);
    void decode_halfword_instruction(uint16_t encoded_inst);
};

inline int inst_size(format_id format) {
    if (format == format_id::half_reg || format == format_id::half_immediate)
        return 2;

    return 4;
}

inline bool is_store_inst(inst_id id) {
    return (id == inst_id::Sb || id == inst_id::Sw || id == inst_id::Sb);
}

inline bool is_load_inst(inst_id id) {
    return (id == inst_id::Lb ||
            id == inst_id::Lh ||
            id == inst_id::Lw ||
            id == inst_id::Lhu ||
            id == inst_id::Lbu);
}

static const std::string_view reg_id_to_string(reg_id id);

static auto string_to_reg_id = enum_name_lookup<reg_id>;

instruction make_set_inst(inst_id id, reg_id dr, int32_t imm);

instruction make_reg_inst(inst_id id, reg_id dr, reg_id sr1, reg_id sr2);

instruction make_immediate_inst(inst_id id, reg_id dr, reg_id sr1, int32_t imm);

instruction make_branch_inst(inst_id id, reg_id sr1, reg_id sr2, int32_t imm);

instruction make_store_inst(inst_id id, reg_id sr1, reg_id sr2, int32_t imm);

instruction make_load_inst(inst_id id, reg_id dr, reg_id sr1, int32_t imm);

instruction make_jump_inst(inst_id id, int32_t imm);

instruction make_half_reg_inst(inst_id id, reg_id dr, reg_id sr);

instruction make_half_imm_inst(inst_id id, reg_id dr, int8_t imm);

const std::string_view inst_id_to_string(inst_id id);
}// namespace isa

inline isa::instruction isa::make_set_inst(inst_id id, reg_id dr, int32_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.immediate = imm;
    inst.dr = dr;
    return inst;
}

inline isa::instruction isa::make_reg_inst(inst_id id, reg_id dr, reg_id sr1, reg_id sr2) {
    instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.dr = dr;
    inst.sr1 = sr1;
    inst.sr2 = sr2;
    return inst;
}

inline isa::instruction isa::make_immediate_inst(inst_id id, reg_id dr, reg_id sr1, int32_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.dr = dr;
    inst.sr1 = sr1;
    inst.immediate = imm;
    return inst;
}

inline isa::instruction isa::make_branch_inst(inst_id id, reg_id sr1, reg_id sr2, int32_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.sr1 = sr1;
    inst.sr2 = sr2;
    inst.immediate = imm;
    return inst;
}

inline isa::instruction isa::make_store_inst(inst_id id, reg_id sr2, reg_id sr1, int32_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.sr1 = sr1;
    inst.sr2 = sr2;
    inst.immediate = imm;
    return inst;
}

inline isa::instruction isa::make_load_inst(inst_id id, reg_id dr, reg_id sr1, int32_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.dr = dr;
    inst.sr1 = sr1;
    inst.immediate = imm;
    return inst;
}

inline isa::instruction isa::make_jump_inst(inst_id id, int32_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.immediate = imm;
    return inst;
}

inline isa::instruction isa::make_half_reg_inst(inst_id id, reg_id dr, reg_id sr) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.dr = dr;
    inst.sr2 = sr;
    return inst;
}

inline isa::instruction isa::make_half_imm_inst(inst_id id, reg_id dr, int8_t imm) {
    isa::instruction inst;
    inst.id = id;
    inst.format = get_inst_type(id).format;
    inst.dr = dr;
    inst.immediate = imm;
    return inst;
}

inline const std::string_view isa::reg_id_to_string(reg_id id) {
    return reg_id_to_str_lut[id];
}

inline const std::string_view isa::inst_id_to_string(inst_id id) {
    return inst_id_to_string_lut[id];
}

inline isa::inst_type isa::get_inst_type(inst_id id) {
    return inst_type_lut[id];
}

inline isa::format_id isa::get_format_from_encoding(uint8_t opcode, instruction_size_type size) {
    for (auto &type: inst_type_lut) {
        if (type.opcode == opcode && (type.is_halfword == (size == instruction_size_type::halfword)))
            return (type.format);
    }

    return format_id::INVALID;
}

inline isa::inst_id isa::get_inst_id_from_encoding(uint8_t opcode, uint8_t funcode, isa::instruction_size_type size) {
    for (int i = 0; i < (int) inst_id::LAST; ++i) {
        const auto &type = inst_type_lut[i];
        if (type.opcode == opcode && (type.is_halfword == (size == instruction_size_type::halfword)) &&
            type.funcode == funcode)
            return ((inst_id) i);
    }

    return inst_id::INVALID;
}

#endif//ASSEMBLER_ISA_H
