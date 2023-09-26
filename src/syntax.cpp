//
// Created by djordy on 12/11/22.
//

#include "syntax.h"
#include "address_counter.h"
#include "isa.h"

std::string parse_string(lexer::token tk) {
    std::string str;
    const auto &text = tk.get_text();

    return str;
}

int64_t syntax::parse_int(lexer::token tk) {
    using token_type = lexer::token::types;

    if(tk.get_type() == token_type::ch) {
        return tk.get_text()[1];
    }

    int base;
    switch (tk.get_type()) {
        case token_type::binary_integer: {
            base = 2;
            break;
        }
        case token_type::decimal_integer: {
            base = 10;
            break;
        }
        case token_type::hex_integer: {
            base = 16;
            break;
        }

        default:
            assert(false);
    }

    return std::stol(tk.get_text(), nullptr, base);
}



syntax::dir_arg syntax::parse_dir_arg(lexer::token tk) {
    dir_arg arg;

    switch (tk.get_type()) {
        case lexer::token::types::hex_integer:
        case lexer::token::types::decimal_integer:
        case lexer::token::types::binary_integer:
        case lexer::token::types::ch:
            arg.type = dir_arg_type::integer;
            arg.int_val = parse_int(tk);
            break;

        case lexer::token::types::label:
            arg.type = dir_arg_type::label;
            arg.identifier = tk.get_text();
            break;

        case lexer::token::types::string:
            arg.type = dir_arg_type::string;
            arg.string = parse_string(tk);
        default:
            assert(false);
    }

    return arg;
}

bool syntax::is_dir_arg(lexer::token tk) {
    return (tk.is_integer() ||
            tk.get_type() == lexer::token::types::label);
}

syntax::inst_arg syntax::parse_inst_arg(lexer::token tk) {
    using token_types = lexer::token::types;
    inst_arg arg;

    if(tk.is_integer()) {
        arg.type = inst_arg_type::integer;
        arg.int_val = parse_int(tk);
    } else if (tk.get_type() == token_types::label) {
        arg.type = inst_arg_type::label;
        arg.label = tk.get_text();
    } else if (tk.get_type() == token_types::reg) {
        arg.type = inst_arg_type::reg;
        arg.reg = tk.get_text();
    }

    return arg;
}

syntax::inst_statement syntax::parse_inst_statement (std::string &&label) {
    using token_types = lexer::token::types;

    enum struct parse_state {
        start, arg, comma, end
    } state = parse_state::start;

    auto tk = lex_m.last_token();
    inst_statement inst_stmnt;

    /*---set label---*/
    inst_stmnt.label = std::move(label);

    //state machine
    while(state != parse_state::end)
        switch (state) {
            case parse_state::start: {
                inst_stmnt.mnemonic = tk.get_text();
                tk = lex_m.fetch_token();
                if(tk.get_type() == lexer::token::types::newline) {
                    state = parse_state::end;
                    break;
                }

                if(is_inst_arg(tk))
                    state = parse_state::arg;
                else
                    state = parse_state::end;
                break;
            }

            case parse_state::arg: {
                inst_stmnt.args.push_back(parse_inst_arg(tk));
                tk = lex_m.fetch_token();
                if(tk.get_type() == token_types::comma)
                    state = parse_state::comma;
                else
                    state = parse_state::end;
                break;
            }

            case parse_state::comma: {
                tk = lex_m.fetch_token();
                if(!is_inst_arg(tk))
                    throw std::runtime_error("expected another argument");
                    //syntax_error("expected another argument");

                state = parse_state::arg;
            }
        }

    return inst_stmnt;
}

syntax::dir_statement syntax::parse_dir_statement(std::string &&label) {
    using token_types = lexer::token::types;

    enum struct parse_state {
        start, arg, comma, end
    } state = parse_state::start;

    auto tk = lex_m.last_token();
    dir_statement dir_stmnt;

    /*---set label---*/
    dir_stmnt.label = std::move(label);

    /*---parse statement---*/
    while(state != parse_state::end)
        switch (state) {
            case parse_state::start: {
                dir_stmnt.directive = tk.get_text();
                tk = lex_m.fetch_token();
                if(tk.get_type() == lexer::token::types::newline) {
                    state = parse_state::end;
                    break;
                }

                if(is_dir_arg(tk))
                    state = parse_state::arg;
                else
                    state = parse_state::end;
                break;
            }

            case parse_state::arg: {
                dir_stmnt.args.push_back(parse_dir_arg(tk));
                tk = lex_m.fetch_token();
                if(tk.get_type() == token_types::comma)
                    state = parse_state::comma;
                else
                    state = parse_state::end;
                break;
            }

            case parse_state::comma: {
                tk = lex_m.fetch_token();
                if(!is_dir_arg(tk))
                    throw std::runtime_error("expected another argument");

                state = parse_state::arg;
            }
        }

    return dir_stmnt;
}

syntax::statement syntax::parse_statement(bool &eof) {
    using token_type = lexer::token::types;
    enum struct parse_state {
        start,
        label,
        colon,
        end
    } state = parse_state::start;

    struct statement stmnt;
    auto tk = lex_m.last_token();
    //eat up new lines
    while(tk.get_type() == lexer::token::types::newline)
        tk = lex_m.fetch_token();

    std::string label = "";

    //check if the lexer has reached the end of the file before attempting to parse another statement
    if(tk.get_type() == token_type::eof) {
        eof = true;
        return stmnt;
    } else {
        eof = false;
    }

    //state machine
    while (state != parse_state::end)
        switch (state) {
            case parse_state::start: {
                if(tk.get_type() == token_type::label) {
                    state = parse_state::label;
                    break;
                }

                if(tk.get_type() == token_type::directive) {
                    stmnt.type = statement_type::dir;
                    stmnt.dir = parse_dir_statement(std::move(label));
                    state = parse_state::end;
                    break;
                }

                if(tk.get_type() == token_type::mnemonic) {
                    stmnt.type = statement_type::inst;
                    stmnt.inst = parse_inst_statement(std::move(label));
                    state = parse_state::end;
                    break;
                }

                throw std::runtime_error("expected label, mnemonic or directive");
            }

            case parse_state::label: {
                label = tk.get_text();
                tk = lex_m.fetch_token();

                if(tk.get_type() != token_type::collon)
                    throw std::runtime_error("missing collon");

                state = parse_state::colon;
                break;
            }

            case parse_state::colon: {
                tk = lex_m.fetch_token();
                //eat up new lines
                while(tk.get_type() == lexer::token::types::newline)
                    tk = lex_m.fetch_token();


                if (tk.get_type() == token_type::directive) {
                    stmnt.type = statement_type::dir;
                    stmnt.dir = parse_dir_statement(std::move(label));
                    state = parse_state::end;
                } else if (tk.get_type() == token_type::mnemonic) {
                    stmnt.type = statement_type::inst;
                    stmnt.inst = parse_inst_statement(std::move(label));
                    state = parse_state::end;
                } else {
                    throw std::runtime_error("expected mnemonic or directive");
                }
                break;
            }
        }

    return stmnt;
}

void syntax::parse_file(std::vector<statement> &tree) {
    bool eof;
    for(auto stmnt = parse_statement(eof); eof != true; stmnt = parse_statement(eof)) {
        tree.push_back(stmnt);
    }
};
