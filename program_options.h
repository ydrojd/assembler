//
// Created by djordy on 1/6/23.
//

#ifndef ASSEMBLER_PROGRAM_OPTIONS_H
#define ASSEMBLER_PROGRAM_OPTIONS_H

#include <filesystem>
#include <map>

class program_options {
public:
    std::filesystem::path input_fname;
    std::filesystem::path output_fname;
    bool short_jumps;
    bool save_pp_result;
    program_options(int argc, char *args[]);

    program_options(const program_options &a) = delete;

private:
    bool is_option_specifier(const std::string &str) {
        return is_option_specifier(str.c_str());
    }

    bool is_option_specifier(const char *str) {
        return str[0] == '-';
    }

    enum struct option_id {
        output = 0,
        short_jump,
        save_pp_result
    };

    static inline std::map<std::string, option_id> option_name_map {
            {"-o", option_id::output},
            {"--savepp", option_id::save_pp_result},
            {"--shortjumps", option_id::short_jump},
    };
};

#endif//ASSEMBLER_PROGRAM_OPTIONS_H
