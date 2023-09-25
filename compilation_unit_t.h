//
// Created by djordy on 1/2/23.
//

#ifndef ASSEMBLER_COMPILATION_UNIT_T_H
#define ASSEMBLER_COMPILATION_UNIT_T_H

#include "isa.h"
#include "symbol_table.h"
#include "binary_data.h"

struct compilation_unit {
    symbol_table st;
    std::vector<binary_data::data_alloc_t> data;
    std::vector<binary_data::data_alloc_t> rodata;
    std::vector<binary_data::data_alloc_t> bss;
    std::vector<isa::instruction> instructions;
};

#endif//ASSEMBLER_COMPILATION_UNIT_T_H
