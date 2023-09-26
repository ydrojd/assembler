//
// Created by djordy on 1/2/23.
//

#ifndef ASSEMBLER_BINARY_GENERATOR_H
#define ASSEMBLER_BINARY_GENERATOR_H

#include "semantic_statement.h"
#include "isa.h"
#include "symbol_table.h"
#include "compilation_unit_t.h"

void binary_generator(const std::string &fname, compilation_unit &comp_unit);


#endif//ASSEMBLER_BINARY_GENERATOR_H
