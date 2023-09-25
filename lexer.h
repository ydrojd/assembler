//
// Created by djordy on 12/11/22.
//

#ifndef ASSEMBLER_LEXER_H
#define ASSEMBLER_LEXER_H

#include "F.h"
#include <string>
#include "magic_enum/magic_enum.hpp"
#include <fstream>
#include <exception>
#include "isa.h"
#include "asm_lang.h"

class lexer : public yyFlexLexer {
    std::fstream file_m;
public:
    class token
    {
        friend lexer;
    public:
        enum struct types
        {
            eof = 0,
            none,
            label,
            directive,
            comma,
            collon,
            hex_integer,
            decimal_integer,
            binary_integer,
            ch,
            reg,
            mnemonic,
            newline,
            string,
            LAST
        };
        token() = default;

    private:
        types id = types::none;
        std::string text_m;

        static const std::string &type_to_string(types typ);

    public:

        bool is_integer() const;
        const types get_type() const { return id; }
        const std::string &get_text() const { return text_m; }
        const std::string to_string() const;
    };

private:
    token read_token_m;

public:
    using yyFlexLexer::yyFlexLexer;
    lexer(std::fstream &str) : lexer(str, std::cout){};
    lexer(std::fstream *str) : lexer(str, &std::cout){};

    token last_token() {return read_token_m;};
    token fetch_token();
};

std::ostream &operator<<(std::ostream &strm, lexer::token &tk);

#endif //ASSEMBLER_LEXER_H
