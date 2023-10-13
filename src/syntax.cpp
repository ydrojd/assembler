//
// Created by djordy on 12/11/22.
//

#include "lexer.h"
#include "syntax.h"
#include "address_counter.h"
#include "isa.h"
#include <cstdint>

static std::pair<int64_t, bool> try_parse_int(lexer::token tk) {
    using token_type = lexer::token::types;

    int64_t int_val = 0;
    switch (tk.get_type()) {
        case token_type::ch:
            int_val = tk.get_text()[1];
            break;

        case token_type::binary_integer:
            int_val = std::stol(tk.get_text(), nullptr, 2);
            break;

        case token_type::decimal_integer:
            int_val = std::stol(tk.get_text(), nullptr, 10);
            break;

        case token_type::hex_integer:
            int_val = std::stol(tk.get_text(), nullptr, 16);
            break;

        default:
            return {0, false};
    }

    return {int_val, true};
}

static std::pair<syntax::arg, bool> try_parse_arg(lexer::token tk) {
    using token_type = lexer::token::types;
    syntax::arg_type type;

    const auto [int_val, is_int] = try_parse_int(tk);
    if (is_int)
	type = syntax::arg_type::integer;
    else {
        switch (tk.get_type()) {
        case token_type::label:
            type = syntax::arg_type::label;
            break;

        case token_type::string:
            type = syntax::arg_type::string;
            break;

        case token_type::reg:
            type = syntax::arg_type::reg;
            break;

        default:
            return {syntax::arg{}, false};
        }
    }

    const auto argument = syntax::arg {type, tk.get_text(), int_val};
    return {argument, true};
}

std::vector<syntax::arg> syntax::parse_arguments() {
    std::vector<syntax::arg> args;
    using token_type = lexer::token::types;

    //---first argument---//
    const auto tk = lex_m.last_token();
    const auto [argument, success] = try_parse_arg(tk);
    if (success == false) throw std::runtime_error("expected argument");
    args.push_back(argument);

    //---argument list---//
    while(true) {
	const auto first_tk = lex_m.fetch_token();
	if (first_tk.get_type() == token_type::newline)
	    break;

	if (first_tk.get_type() != token_type::comma)
	    throw std::runtime_error(std::string{"expected comma. Got: "} + first_tk.to_string());
	
	const auto second_tk = lex_m.fetch_token();
	const auto [argument, success] = try_parse_arg(second_tk);
	if (success == false) throw std::runtime_error("expected another argument");
	args.push_back(argument);
    }

    return args;
}

void syntax::eat_whitelines(void) {
    auto tk = lex_m.last_token();
    while(tk.get_type() == lexer::token::types::newline)
        tk = lex_m.fetch_token();
}

std::pair<syntax::statement, bool> syntax::parse_statement() {
    using token_type = lexer::token::types;

    //---whitelines---//
    eat_whitelines();
    auto tk = lex_m.last_token();

    //check if the lexer has reached the end of the file before attempting to parse another statement
    const bool eof = (tk.get_type() == token_type::eof);
    if (eof) return {syntax::statement {}, eof};

    struct statement stmnt;

    //---label---//
    if (tk.get_type() == token_type::label) {
        stmnt.label = tk.get_text();
        tk = lex_m.fetch_token();
        if (tk.get_type() != token_type::collon)
	    throw std::runtime_error(std::string{"missing collon. Got: "} + tk.to_string());

	tk = lex_m.fetch_token();
    }

    //---whitelines---//
    eat_whitelines();
    tk = lex_m.last_token();

    //---directive/statement--//
    if (tk.get_type() == token_type::directive) {
	stmnt.type = statement_type::dir;
	stmnt.directive = tk.get_text();
    } else if (tk.get_type() == token_type::mnemonic) {
        stmnt.type = statement_type::inst;
	stmnt.mnemonic = tk.get_text();
    } else throw std::runtime_error(std::string{"expected label, mnemonic or directive. Got: "} + tk.to_string());

    //---arguments---//
    tk = lex_m.fetch_token();
    if(tk.get_type() != token_type::newline)
	stmnt.args = parse_arguments();

    return {stmnt, eof};
}

void syntax::parse_file(std::vector<statement> &tree) {
    for (auto ret = parse_statement(); ret.second != true; ret = parse_statement())
        tree.push_back(ret.first);
}
