cmake_minimum_required(VERSION 3.20)
project(assembler)

add_executable(assembler)

SET_SOURCE_FILES_PROPERTIES(
        ./gpp/libgpp.a
        PROPERTIES
        EXTERNAL_OBJECT true
        GENERATED true
)

find_package(FLEX REQUIRED)

flex_target(lexer ./src/F.l F.cpp DEFINES_FILE ./incl/F.h)

target_compile_features(assembler PRIVATE cxx_std_23)

target_link_libraries(assembler PRIVATE gpp)

add_subdirectory(gpp)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

target_sources(assembler PRIVATE
  ./main.cpp
  F.cpp ./incl/F.h

  ./src/isa.cpp ./incl/isa.h
  ./src/asm_lang.cpp ./incl/asm_lang.h
  ./src/binary_generator.cpp ./incl/binary_generator.h
  ./src/address_counter.cpp ./incl/address_counter.h
  ./src/elf_generator.cpp ./incl/elf_generator.h
  ./src/lexer.cpp ./incl/lexer.h
  ./src/program_options.cpp ./incl/program_options.h
  ./src/semantic_analyzer.cpp ./incl/semantic_analyzer.h
  ./src/semantic_statement.cpp ./incl/semantic_statement.h
  ./src/symbol_table.cpp ./incl/symbol_table.h
  ./src/syntax.cpp ./incl/syntax.h
  
  ./incl/binary.h
  ./incl/binary_data.h
  ./incl/templates.h
)

include_directories(
  ./incl/
  ./
)
