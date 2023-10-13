//
// Created by djordy on 12/11/22.
//

#include "lexer.h"

lexer::token lexer::fetch_token() {
    if (yyin.eof()) {
        read_token_m.id = token::types::eof;
        read_token_m.text_m = "";
	read_token_m.line = yylineno;

        return read_token_m;
    }

    auto token_type = (token::types) yylex();

    if (token_type == token::types::none)
        return fetch_token();

    read_token_m.id = token_type;
    read_token_m.text_m = yytext;
    read_token_m.line = yylineno;

    return read_token_m;
}

const std::string &lexer::token::type_to_string(types typ) {
    static const std::string lut[] = {
            "EOF",
            "none",
            "label",
            "directive",
            "comma",
            "collon",
            "hex_integer",
            "decimal_integer",
            "binary_integer",
            "char",
            "reg",
            "mnemonic",
            "newline"};

    if (typ >= types::LAST)
        throw "invalid op_type value";

    return lut[(int) typ];
}

bool lexer::token::is_integer() const {
    return (id == types::binary_integer ||
            id == types::hex_integer ||
            id == types::decimal_integer ||
            id == types::ch);
}

const std::string lexer::token::to_string() const {
    std::string str{type_to_string(id)};
    str += ": ";
    str += text_m;
    return str;
}

std::ostream &operator<<(std::ostream &strm, lexer::token &tk) {
    strm << tk.to_string();

    return strm;
}
