//
// Created by djordy on 1/6/23.
//

#include "program_options.h"

program_options::program_options(int argc, char **args) {

    //---set default options---//
    short_jumps = false;
    save_pp_result = false;

    //---parse options---//
    bool input_set = false;
    bool output_set = false;

    argc--;
    args++;
    while (argc != 0) {
        if(is_option_specifier(*args)) {
            const auto search_it = option_name_map.find(*args);
            if(search_it == option_name_map.end())
                throw std::runtime_error("unrecognized option");

            const auto option = search_it->second;
            switch (option) {
                case option_id::save_pp_result:
                    save_pp_result = true;
                    break;
                case option_id::short_jump:
                    short_jumps = true;
                    break;
                case option_id::output:
                    argc--;
                    args++;
                    if(argc == 0 || is_option_specifier(*args))
                        throw std::runtime_error("missing filename after -o");

                    if(output_set == false) {
                        output_set = true;
                        output_fname = *args;
                    } else {
                        throw std::runtime_error("output already specified");
                    }
                    break;
            }
        } else {
            if(input_set == false) {
                input_fname = *args;
                input_set = true;
            } else
                throw std::runtime_error("input file specified more than ones");
        }

        argc--;
        args++;
    }

    if(input_set == false)
        throw std::runtime_error("no input file");

    if(output_set == false) {
        output_fname = input_fname;
        output_fname.replace_extension(".elf");
    }
}
