//
// Created by djordy on 12/15/22.
//

#include "isa.h"
#include "bitwise_functions.h"

uint32_t set_bit_extension(uint32_t val, uint8_t pos, isa::extension_type ext_type) {
    uint32_t result;

    switch (ext_type) {
        case isa::extension_type::one:
            result = bit_extend(val, true, pos);
            break;
        case isa::extension_type::zero:
            result = bit_extend(val, false, pos);
            break;
        case isa::extension_type::sign:
            result = signed_bit_extend(val, pos);
            break;
        case isa::extension_type::na:
            result = val;
            break;
        default:
            assert(!"unreachable");
    }

    return result;
}

isa::encoded_instruction isa::instruction::encode() const {
    assert(id != inst_id::INVALID);

    encoded_instruction encoded;
    auto inst_info = get_inst_type(id);

    encoded.fullword = 0;

    if (inst_info.is_halfword) {
        /*---half word---*/
        encoded.size = instruction_size_type::halfword;

        /*---bitmode---*/
        bitwise_place(encoded.halfword, 0, bitmode_pos, bitmode_bitsize);

        /*--set opcode---*/
        bitwise_place(encoded.halfword, inst_info.opcode, opcode_pos, opcode_bitsize);

        if (inst_info.format == format_id::half_immediate) {
            /*---set immediate---*/
            auto imm = bitwize_select(immediate, 0, halfword_immediate_bitsize);
            bitwise_place(encoded.halfword, imm, halfword_immediate_pos, halfword_immediate_bitsize);
        } else {
            /*---set sr---*/
            assert(inst_info.format == format_id::half_reg);
            bitwise_place(encoded.halfword, (uint16_t) sr2, halfword_sr_pos, reg_bitsize);
        }

        /*---set dr---*/
        bitwise_place(encoded.halfword, (uint16_t) dr, halfword_dr_pos, reg_bitsize);

    } else {
        /*---full word---*/
        encoded.size = instruction_size_type::fullword;

        /*---set bitmode---*/
        bitwise_place(encoded.fullword, 1, bitmode_pos, bitmode_bitsize);

        /*--set opcode---*/
        bitwise_place(encoded.fullword, inst_info.opcode, opcode_pos, opcode_bitsize);

        /*---set fun---*/
        if (inst_info.format == format_id::reg)
            bitwise_place(encoded.fullword, inst_info.funcode, fun_pos, longfun_bitsize);
        else
            bitwise_place(encoded.fullword, inst_info.funcode, fun_pos, shortfun_bitsize);

        auto operand_form = format_operand_form_lut[format];

        /*---set sr2---*/
        if (operand_form.sr2)
            bitwise_place(encoded.fullword, (uint16_t) sr2, sr2_pos, reg_bitsize);

        /*---set sr1---*/
        if (operand_form.sr1)
            bitwise_place(encoded.fullword, (uint16_t) sr1, sr1_pos, reg_bitsize);

        /*---set dr---*/
        if (operand_form.dr)
            bitwise_place(encoded.fullword, (uint16_t) dr, dr_pos, reg_bitsize);

        /*--set immediate---*/
        if(operand_form.imm)
            encode_fullword_immediate(encoded.fullword);

    }

    return encoded;
}

void isa::instruction::decode_fullword_instruction(uint32_t encoded_inst) {
    /*---opcode---*/
    auto opcode = bitwize_select(encoded_inst, opcode_pos, opcode_bitsize);

    /*--format---*/
    format = get_format_from_encoding(opcode, instruction_size_type::fullword);

    //handle undefined instruction
    if(format == format_id::INVALID) {
        id = inst_id::INVALID;
        return;
    }

    /*---funcode--*/
    uint8_t funcode = 0;
    if (format == format_id::immediate ||
        format == format_id::branch)
        funcode = bitwize_select(encoded_inst, fun_pos, shortfun_bitsize);
    else if (format == format_id::reg)
        funcode = bitwize_select(encoded_inst, fun_pos, longfun_bitsize);
    else
        funcode = 0;

    /*---get id---*/
    this->id = get_inst_id_from_encoding(opcode, funcode, instruction_size_type::fullword);

    //handle undefined instruction
    if(id == inst_id::INVALID)
        return;

    auto operand_form = format_operand_form_lut[format];

    /*---get sr2---*/
    if (operand_form.sr2)
        sr2 = (reg_id) bitwize_select(encoded_inst, sr2_pos, reg_bitsize);
    else
        sr2 = reg_id::zero;

    /*---get sr1---*/
    if (operand_form.sr1)
        sr1 = (reg_id) bitwize_select(encoded_inst, sr1_pos, reg_bitsize);
    else
        sr1 = reg_id::zero;

    /*---get dr---*/
    if (operand_form.dr)
        dr = (reg_id) bitwize_select(encoded_inst, dr_pos, reg_bitsize);
    else
        dr = reg_id::zero;

    //set dr to return address for rjali
    if(id == isa::inst_id::Rjali)
        dr = reg_id::ra;

    /*--get immediate---*/
    decode_fullword_immediate(encoded_inst);
};

void isa::instruction::decode_halfword_instruction(uint16_t encoded_inst) {
    /*---opcode---*/
    auto opcode = bitwize_select(encoded_inst, opcode_pos, opcode_bitsize);

    /*--format---*/
    format = get_format_from_encoding(opcode, instruction_size_type::halfword);
    //handle undefined instruction
    if(format == format_id::INVALID) {
        id = inst_id::INVALID;
        return;
    }

    /*--get id---*/
    this->id = get_inst_id_from_encoding(opcode, 0, instruction_size_type::halfword);
    if(id == inst_id::INVALID)
        return;

    /*---get dr---*/
    dr = (reg_id) bitwize_select(encoded_inst, halfword_dr_pos, reg_bitsize);

    /*---get sr1--*/
    if(id == inst_id::Mov || id == inst_id::Jalr)
        sr1 = reg_id::zero;
    else
        sr1 = dr;

    /*---get sr2---*/
    if (format == format_id::half_reg)
        sr2 = (reg_id) bitwize_select(encoded_inst, halfword_sr_pos, reg_bitsize);
    else
        sr2 = reg_id::zero;

    /*---get imm---*/
    if (format == format_id::half_immediate) {
        immediate = bitwize_select(encoded_inst, isa::halfword_immediate_pos, isa::reg_bitsize);
        immediate = set_bit_extension(immediate, isa::reg_bitsize, get_inst_type(id).extension);
    } else {
        immediate = 0;
    }
}


isa::instruction::instruction(isa::encoded_instruction encoded) {
    switch (encoded.size) {
        case instruction_size_type::fullword:
            decode_fullword_instruction(encoded.fullword);
            break;
        case instruction_size_type::halfword:
            decode_halfword_instruction(encoded.halfword);
            break;
        default:
            assert(false);
    }
}

void isa::instruction::decode_fullword_immediate(uint32_t encoded_inst) {
    auto inst_info = get_inst_type(id);

    switch (inst_info.format) {
        case format_id::immediate:
            immediate = bitwize_select(encoded_inst, imm_immediate_pos, imm_immediate_bitsize);
            immediate = set_bit_extension(immediate, imm_immediate_bitsize, inst_info.extension);
            break;
        case format_id::branch:
            immediate = bitwize_select(encoded_inst, branch_lowerimmediate_pos, branch_lowerimmediate_bitsize);
            immediate += bitwize_select(encoded_inst, branch_upperimmediate_pos, branch_upperimmediate_bitsize)
                    << branch_lowerimmediate_bitsize;
            immediate = set_bit_extension(immediate, branch_lowerimmediate_bitsize + branch_upperimmediate_bitsize,
                                          inst_info.extension);

            if(isa::is_store_inst(id) == false)
                immediate = immediate << 1;

            break;
        case format_id::set:
            immediate = bitwize_select(encoded_inst, set_immediate_pos, set_immediate_bitsize);
            if(id == inst_id::Sui || id == inst_id::Apci) {
                immediate  = immediate << (32 - isa::set_immediate_bitsize);
            } else {
                immediate = set_bit_extension(immediate, set_immediate_bitsize, inst_info.extension);
            }
            break;
        case format_id::jump:
            immediate = bitwize_select(encoded_inst, jump_immediate_pos, jump_immediate_bitsize);
            immediate = set_bit_extension(immediate, jump_immediate_bitsize, inst_info.extension);
            immediate = immediate << 1;
            break;
        default:
            immediate = 0;
    }
}

std::string isa::instruction::to_str() const {
    /*---invalid/nop---*/
    if(id == inst_id::INVALID)
        return "NOT DEFINED";
    else if(id == inst_id::Add_h &&
            dr == reg_id::zero &&
            sr1 == reg_id::zero &&
            sr2 == reg_id::zero)
        return "NOP";

    std::string str;
    auto operand_form = format_operand_form_lut[format];

    /*---mnemonic---*/
    str.append(inst_id_to_string(id));

    /*---registers---*/
    bool reg_appended = false;
    if(operand_form.dr) {
        str.append(" ");
        str.append(reg_id_to_string(dr));
        reg_appended = true;
    }

    if(operand_form.sr1) {
        str.append(reg_appended ? ", " : " ");
        str.append(reg_id_to_string(sr1));
        reg_appended = true;
    }

    if(operand_form.sr2) {
        str.append(reg_appended ? ", " : " ");
        str.append(reg_id_to_string(sr2));
        reg_appended = true;
    }

    /*---immediate---*/
    if(operand_form.imm) {
        str.append(reg_appended ? ", " : " ");
        str.append(std::to_string(immediate));
    }

    return str;
}

void isa::instruction::encode_fullword_immediate(uint32_t &inst) const {
    switch (format) {
        case format_id::branch: {
            const auto shifted_immediate = isa::is_store_inst(id) ? immediate : (immediate / 2);
            const auto lowerimm = bitwize_select(shifted_immediate, 0, branch_lowerimmediate_bitsize);
            bitwise_place(inst, lowerimm, branch_lowerimmediate_pos, branch_lowerimmediate_bitsize);
            const auto upperimm = bitwize_select(shifted_immediate, branch_lowerimmediate_bitsize, branch_upperimmediate_bitsize);
            bitwise_place(inst, upperimm, branch_upperimmediate_pos, branch_upperimmediate_bitsize);
            break;
        }

        case format_id::immediate: {
            const auto imm = bitwize_select(immediate, 0, imm_immediate_bitsize);
            bitwise_place(inst, imm, imm_immediate_pos, imm_immediate_bitsize);
            break;
        }

        case format_id::set: {
            const auto imm = bitwize_select(immediate, 0, set_immediate_bitsize);
            bitwise_place(inst, imm, set_immediate_pos, set_immediate_bitsize);
            break;
        }

        case format_id::jump: {
            const auto shifted_immediate = immediate / 2;
            const auto imm = bitwize_select(shifted_immediate, 0, jump_immediate_bitsize);
            bitwise_place(inst, imm, jump_immediate_pos, jump_immediate_bitsize);
            break;
        }
        default:
            break;
    }
}
