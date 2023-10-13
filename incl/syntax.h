//
// Created by djordy on 12/11/22.
//

#ifndef ASSEMBLER_SYNTAX_H
#define ASSEMBLER_SYNTAX_H

#include <cstdint>
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

    enum struct arg_type {
        integer,
        label,
        reg,
	string,
    };

    struct arg {
	arg_type type;
	std::string str_val;
	int64_t int_val;
    };
    
    enum struct statement_type {
        dir,
        inst
    };

    struct statement {
        statement_type type;
        std::string label;
        std::string mnemonic;
	std::string directive;
        std::vector<arg> args;
    };

    std::pair<syntax::statement, bool> parse_statement();
    void parse_file(std::vector<statement> &tree);

private:
    std::vector<syntax::arg> parse_arguments();
    void eat_whitelines(void);
};

#endif //ASSEMBLER_SYNTAX_H
