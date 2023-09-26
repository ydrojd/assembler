//
// Created by djordy on 12/11/22.
//

#ifndef ASSEMBLER_SYNTAX_H
#define ASSEMBLER_SYNTAX_H

#include <vector>

#include "lexer.h"
#include "symbol_table.h"
#include "address_counter.h"

class syntax {
private:
    lexer lex_m;
public:
    syntax(std::fstream &f) : lex_m(f) {
        lex_m.fetch_token();
    };

    syntax(std::fstream *f) : lex_m(f) {
        lex_m.fetch_token();
    };

    enum struct dir_arg_type {
        integer,
        label,
        string,
    };

    struct dir_arg {
        dir_arg_type type;
        std::string identifier;
        int64_t int_val = 0;
        std::string string;
    };

    enum struct inst_arg_type {
        integer,
        label,
        reg,
    };

    struct inst_arg {
        inst_arg_type type;

        int64_t int_val = 0;
        std::string label{""};
        std::string reg{""};
    };

    struct inst_statement {
        std::string label;
        std::string mnemonic;
        std::vector<inst_arg> args;
    };

    struct dir_statement {
        std::string label;
        std::string directive;
        std::vector <dir_arg> args;
    };

    enum struct statement_type {
        dir,
        inst
    };

    struct statement {
        statement_type type;
        inst_statement inst;
        dir_statement dir;
    };


    bool is_inst_arg (lexer::token tk) {
        return (tk.is_integer() ||
                tk.get_type() == lexer::token::types::label ||
                tk.get_type() == lexer::token::types::reg);
    };

    dir_arg parse_dir_arg(lexer::token tk);
    bool is_dir_arg(lexer::token tk);
    int64_t parse_int(lexer::token tk);
    inst_arg parse_inst_arg(lexer::token tk);
    dir_statement parse_dir_statement(std::string &&label);
    inst_statement parse_inst_statement(std::string &&label);
    statement parse_statement(bool &eof);
    void parse_file(std::vector<statement> &tree);
};

#endif //ASSEMBLER_SYNTAX_H
