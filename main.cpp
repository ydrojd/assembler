#include <iostream>

#include "semantic_analyzer.h"
#include "binary_generator.h"
#include "isa.h"
#include "semantic_statement.h"
#include "syntax.h"
#include "program_options.h"

#include <boost/filesystem.hpp>
#include <filesystem>
#include <boost/interprocess/streams/bufferstream.hpp>

extern "C" int gpp(int argc, char **argv, FILE *output_file, FILE *input_file);


int main(int argc, char *args[]) {
    auto options = program_options(argc, args);

    //---run gpp preprocessor---//
    FILE *macro_input = fopen(options.input_fname.c_str(), "r");
    if(macro_input == nullptr)
        throw std::runtime_error("could not read input file");

    size_t size;
    char *ptr;
    FILE *output_assembly = open_memstream(&ptr, &size);
    assert(output_assembly);

    gpp(1, args + argc - 1, output_assembly, macro_input);
    boost::interprocess::bufferstream assembly(ptr, size);

    //---compile---//
    syntax parser((std::fstream *) &assembly); //syntax parser
    auto compile_unit = semantic_analyzer(parser, options); //semantic_statements analysis
    binary_generator(options.output_fname, compile_unit); //elf binary generator

    if(options.save_pp_result) {
        //---save preprocessor output---//
        std::filesystem::path pp_output_path = options.input_fname;
        pp_output_path.replace_extension(".pp.s");
        FILE *f = fopen(pp_output_path.c_str(), "w");
        fputs(ptr, f);
        fclose(f);
    }

    return 0;
}

