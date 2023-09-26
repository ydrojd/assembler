//
// Created by djordy on 1/2/23.
//

#ifndef ASSEMBLER_SEMANTIC_ANALYZER_H
#define ASSEMBLER_SEMANTIC_ANALYZER_H

#include "binary_data.h"
#include "compilation_unit_t.h"
#include "program_options.h"
#include "semantic_statement.h"
#include "symbol_table.h"

compilation_unit semantic_analyzer(syntax &parser, program_options &options);

#endif//ASSEMBLER_SEMANTIC_ANALYZER_H
