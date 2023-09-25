cmake_minimum_required(VERSION 3.21)
project(assembler)

find_package(FLEX REQUIRED)
flex_target(lexer F.l F.cpp DEFINES_FILE ../F.h)

SET(OBJS ${CMAKE_CURRENT_SOURCE_DIR}/gpp.o)

SET_SOURCE_FILES_PROPERTIES(
        ${OBJS}
        PROPERTIES
        EXTERNAL_OBJECT true
        GENERATED true
)

set(CMAKE_CXX_STANDARD 23)
add_executable(assembler ${OBJS} main.cpp program_options.cpp program_options.h isa.h F.cpp F.h lexer.cpp lexer.h syntax.cpp syntax.h semantic_statement.cpp semantic_statement.h symbol_table.cpp symbol_table.h address_counter.cpp address_counter.h asm_lang.cpp asm_lang.h binary.h isa.cpp templates.h bitwise_functions.h elf_generator.cpp elf_generator.h binary_data.h binary.h semantic_analyzer.cpp semantic_analyzer.h binary_generator.cpp binary_generator.h compilation_unit_t.h)
target_link_libraries(${PROJECT_NAME} ncurses)
