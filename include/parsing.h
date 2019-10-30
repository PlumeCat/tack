#ifndef _PARSING_H
#define _PARSING_H

// Parse context structure; better than passing "char*&" everywhere
class parse_end : public exception {};
class invalid_program : public exception {};

struct parse_context {
    string::const_iterator at;
    string::const_iterator end;

    parse_context() = delete;
    parse_context(const parse_context&) = default;
    parse_context(const string& str) {
        at = str.begin();
        end = str.end();
    }
    parse_context& operator=(const parse_context&) = default;

    char peek() {
        if (at == end) {
            throw parse_end();
        }
        // if (*at == '\r') {
        //     // TODO: ugly hack, literally ignore '\r' characters altogether
        //     advance();
        //     return peek();
        // }
        return *at;
    }
    void advance(int n = 1) {
        if (end - at < n) {
            throw parse_end();
        }
        at += n;
    }
};


// Parsing DSL
#define _cat(a, b) a##b
#define cat(a, b) _cat(a, b)
#define rulename(rule) cat(parse_, rule)
// use this for forward declarations
#define DECLARE_RULE(name)\
bool rulename(name)(parse_context& ctx, ast& result)

// begin parsing function
#define DEFINE_RULE(name)\
DECLARE_RULE(name) {\
    auto _eof = false;
#define TRY(exp, stuff) \
    auto cat(_restore, __LINE__) = ctx.at;\
    try {\
        if (true exp) { \
            stuff\
            return true; \
        }\
        ctx.at = cat(_restore, __LINE__);\
    } catch (parse_end&) {\
        ctx.at = cat(_restore, __LINE__);\
        _eof = true; /* eof was encountered trying to parse this rule */ \
    }
#define END_RULE()\
    if (_eof) {\
        /* nothing succeeded but one or more of them threw EOF; pass upwards */ \
        throw parse_end();\
    }\
    return false;\
}

// parsing DSL "combinators"
#define WS                  && parse_ws(ctx)
#define CHAR(c)             && parse_char(ctx, c)
#define STR(s)              && parse_string(ctx, s)
#define RULE(rule, res)     && rulename(rule)(ctx, res)
#define TERM                && parse_terminator(ctx)



//////////////////////////////////////////////////////////////////////////
// Parsing simple terminals
//////////////////////////////////////////////////////////////////////////


// consume valid identifier
// return false if identifier not found
// if a valid starting character (_a-zA_Z) is encountered,
//      EOF will be acceptable from that point onward
bool parse_identifier(parse_context& ctx, string& name) {
    auto start = ctx.at;
    auto f = ctx.peek();
    if (f == '_' ||
        (f >= 'A' && f <= 'Z') ||
        (f >= 'a' && f <= 'z')) {
        ctx.advance();

        try {
            // eof terminating identifier is ok
            while (true) {
                auto c = ctx.peek();
                if (c != '_' &&
                    !(c >= 'A' && c <= 'Z') &&
                    !(c >= 'a' && c <= 'z') &&
                    !(c >= '0' && c <= '9')) {
                    break;
                }
                ctx.advance();
            }
        } catch (parse_end&) {}

        name = string(start, ctx.at);
        return true;
    }

    ctx.at = start;
    return false;
}

// consume a number and leading whitespace - integer format only
// return false if not found
// throw if EOF
bool parse_integer(parse_context& ctx, double& num) {
    const auto start_pos = ctx.at;
    auto f = ctx.peek();
    if (f >= '0' && f <= '9') {
        ctx.advance();
        try {
            while (true) {
                auto c = ctx.peek();
                if (!(c >= '0' && c <= '9')) {
                    break;
                }
                ctx.advance();
            }
        } catch (parse_end&) {}

        num = (double)stoi(string(start_pos, ctx.at), nullptr);
        return true;
    }

    ctx.at = start_pos;
    return false;
}

// consume the given character
// return false if not found
// throw if EOF
bool parse_char(parse_context& ctx, char c) {
    if (ctx.peek() != c) {
        return false;
    }
    ctx.advance();
    return true;
}

// consume the statement / expression terminator (and any leading whitespace)
// return true if found
// can be ';', '\n' or EOF (ergo, doesn't throw on EOF)
// this has the nice property of transparently skipping the '\r' characters in CRLF files
bool parse_terminator(parse_context& ctx) {
    auto restore = ctx.at;
    while (true) {
        try {
            auto c = ctx.peek();
            if (c == ';' || c == '\n') {
                ctx.advance(); // won't throw
                return true;
            }
            if (isspace(c)) {
                ctx.advance(); // won't throw
            } else {
                // no terminator found
                break;
            }
        } catch (parse_end&) {
            return true; // EOF is ok
        }
    }
    ctx.at = restore;
    return false;
}

// consume 0 or more whitespace chars
// always returns true
// will not throw EOF
bool parse_ws(parse_context& ctx) {
    try {
        while (isspace(ctx.peek())) {
            ctx.advance();
        }
    } catch (parse_end&) {}
    return true;
}

// consume the given string
template<int N> bool parse_string(parse_context& ctx, const char(&str)[N]) {
    assert(str[N-1] == 0);
    auto restore = ctx.at;
    for (int i = 0; i < N-1; i++) {
        auto c = ctx.peek();
        if (c != str[i]) {
            ctx.at = restore;
            return false;
        }
        ctx.advance();
    }
    return true;
}

// consume a number (and leading whitespace)
// return false if not found
// throw if EOF
bool parse_number(parse_context& ctx, double& num) {
    // TODO: dodgy address-of-reference?
    const auto sp = ctx.at;
    const auto start_pos = &*ctx.at;
    auto end_pos = (char*)nullptr;
    num = strtof(start_pos, &end_pos);
    if (end_pos == start_pos) {
        return false;
    }
    ctx.advance(end_pos - start_pos);
    return true;
}

// parse a literal string - anything enclosed by '"'
// will throw on EOF if the opening " is not closed
bool parse_string_literal(parse_context& ctx, string& out_str) {
    // const auto start_pos = &*ctx.at;
    // auto end_pos = (char*)nullptr;
    if (ctx.peek() != '"') {
        return false;
    }
    ctx.advance();
    auto first = ctx.at;
    while (ctx.peek() != '"') {
        ctx.advance();
    }
    out_str = string(first, ctx.at);
    ctx.advance(); // skip the last '"'
    return true;
}

//////////////////////////////////////////////////////////////////////////
// Parsing operators
//////////////////////////////////////////////////////////////////////////

// consume the declaration operator
// return false if not found
// throw if EOF
bool parse_declare_op(parse_context& ctx, ast_operator& result) {
    if (parse_char(ctx, '=')) {
        result = OP_ASSIGN;
        return true;
    }
    result = OP_UNKNOWN;
    return false;
}

// consume assignment operators, if any
// return false if not found
// throw if eof
bool parse_assign_op(parse_context& ctx, ast_operator& result) {
    // see comments in parse_bin_op
    result = parse_char(ctx, '=') ? OP_ASSIGN :
            parse_string(ctx, "+=") ? OP_ASSIGN_ADD :
            parse_string(ctx, "-=") ? OP_ASSIGN_SUB :
            parse_string(ctx, "*=") ? OP_ASSIGN_MUL :
            parse_string(ctx, "/=") ? OP_ASSIGN_DIV :
            parse_string(ctx, "%=") ? OP_ASSIGN_MOD :
            parse_string(ctx, "|=") ? OP_ASSIGN_OR :
            parse_string(ctx, "&=") ? OP_ASSIGN_AND :
            parse_string(ctx, "^=") ? OP_ASSIGN_XOR :
            parse_string(ctx, "+=") ? OP_ASSIGN_LSH :
            parse_string(ctx, "+=") ? OP_ASSIGN_RSH :
            OP_UNKNOWN;
    return result != OP_UNKNOWN;
}

// consume an unary operator
// return false if not found
// throw if EOF
bool parse_unary_op(parse_context& ctx, ast_operator& result) {
    // no need to catch EOF; if one throws it all would throw it
    result = parse_char(ctx, '+') ? OP_ADD :
            parse_char(ctx, '-') ? OP_SUB :
            parse_char(ctx, '!') ? OP_BOOL_NOT :
            parse_char(ctx, '~') ? OP_NOT :
            OP_UNKNOWN;
    return (result != OP_UNKNOWN);
}

// consume a binary operator
// return false if not found
// throw if EOF
bool parse_bin_op(parse_context& ctx, ast_operator& result) {
    // no need to catch EOF; if one throws it they all would throw it
    // just make sure to try them in reverse order of length to preserve this
    // eg "<<=" might throw EOF when "+1" would not
    result = parse_char(ctx, '+') ? OP_ADD :
            parse_char(ctx, '-') ? OP_SUB :
            parse_char(ctx, '*') ? OP_MUL :
            parse_char(ctx, '/') ? OP_DIV :
            parse_char(ctx, '%') ? OP_MOD :
            OP_UNKNOWN;
    return result != OP_UNKNOWN;
}


//////////////////////////////////////////////////////////////////////////
// Parsing expressions
//////////////////////////////////////////////////////////////////////////

// forward declaration
DECLARE_RULE(exp);
DECLARE_RULE(program);
DECLARE_RULE(program_part);
DECLARE_RULE(block);

DEFINE_RULE(func_body)
    TRY(WS
        STR("=>") WS
        RULE(exp, result)
        , {})
END_RULE()

DEFINE_RULE(func_literal)
    log_func();

    auto body = ast();
    log.debug() << "trying unargumented func...";
    TRY(WS
        CHAR('(')       WS
        CHAR(')')       WS
        RULE(func_body, body)
        , {
            log.debug() << "success" << endl;
            result.type = FUNC_LITERAL;
            // no parameter function
            result.children.push_back(body);
        })

    log.debug() << "failed" << endl;

    log.debug() << "trying argumented func {" << string(ctx.at, ctx.at + 10) << "}" << endl;
    auto r = ctx.at;
    if (parse_ws(ctx) && parse_char(ctx, '(')) {
        log.debug() << "  " << "found (" << endl;
        while (true) {
            auto p = ast();
            if (parse_ws(ctx) && parse_identifier(ctx, p.str_data)) {
                log.debug() << "found first identifier: " << p << endl;
                vector<ast> identifiers;

                p.type = IDENTIFIER;
                identifiers.push_back(p);

                // greedily consume identifiers
                while (true) {
                    parse_ws(ctx);
                    auto id = ast();
                    auto body = ast();
                    if (parse_char(ctx, ',') && parse_ws(ctx) && parse_identifier(ctx, id.str_data)) {
                        log.debug() << "found additional identifier: " << id << endl;
                        id.type = IDENTIFIER;
                        identifiers.push_back(id);
                    } else if (parse_char(ctx, ')') && parse_func_body(ctx, body)) {
                        log.debug() << "end of parameter list" << endl;
                        // end of parameter list
                        for (auto& i: identifiers) {
                            result.children.push_back(i);
                        }
                        result.children.push_back(body);
                        result.type = FUNC_LITERAL;
                        return true;
                    } else {
                        ctx.at = r;
                        return false;
                    }
                }
            } else {
                ctx.at = r;
                return false;
            }
        }
    } else {
        ctx.at = r;
        return false;
    }
END_RULE()

DEFINE_RULE(range_literal)
    auto n1 = 0.0;
    auto n2 = 0.0;
    // cout << "range literal: " << string(ctx.at, ctx.end) << endl;
    TRY(WS
        RULE(integer, n1)
        STR("..")
        RULE(integer, n2)
        , {
            result.type = RANGE_LITERAL;
            auto c1 = ast();
            auto c2 = ast();
            c1.type = NUM_LITERAL;
            c2.type = NUM_LITERAL;
            c1.num_data = n1;
            c2.num_data = n2;
            result.children.push_back(c1);
            result.children.push_back(c2);
        })
END_RULE()

DEFINE_RULE(list_literal)
    // empty list
    TRY(WS
        CHAR('[') WS
        CHAR(']')
        , {
            result.type = LIST_LITERAL;
        })

    // non-empty list
    auto r = ctx.at;
    if (parse_ws(ctx) && parse_char(ctx, '[')) {
        while (true) {
            auto p = ast();
            if (parse_ws(ctx) && parse_exp(ctx, p)) {
                vector<ast> elements;
                result.type = LIST_LITERAL;
                elements.push_back(p);

                while (true) {
                    parse_ws(ctx);
                    auto e = ast();
                    if (parse_char(ctx, ',') && parse_ws(ctx) && parse_exp(ctx, e)) {
                        elements.push_back(e);
                    } else if (parse_char(ctx, ']')) {
                        for (auto& e: elements) {
                            result.children.push_back(e);
                        }
                        return true;
                    } else {
                        ctx.at = r;
                        return false;
                    }
                }
            } else {
                ctx.at = r;
                return false;
            }
        }
    } else {
        ctx.at = r;
        return false;
    }
END_RULE()

DEFINE_RULE(if_exp)
    auto e = ast();
    auto yes = ast();
    auto no  = ast();

    // if {} else if {} ...
    TRY(WS
        STR("if")           WS
        RULE(exp, e)        WS
        RULE(block, yes)    WS
        STR("else")         WS
        RULE(if_exp, no)
        , {
            result.type = IF_EXP;
            result.children.push_back(e);
            result.children.push_back(yes);
            result.children.push_back(no);
        })

    // // if {} else {}
    e = ast(); yes = ast(); no = ast();
    TRY(WS
        STR("if")           WS
        RULE(exp, e)        WS
        RULE(block, yes)    WS
        STR("else")         WS
        RULE(block, no)
        , {
            result.type = IF_EXP;
            result.children.push_back(e);
            result.children.push_back(yes);
            result.children.push_back(no);
        })

    // // if {}
    e = ast(); yes = ast();
    TRY(WS
        STR("if")           WS
        RULE(exp, e)        WS
        RULE(block, yes)
        , {
            result.type = IF_EXP;
            result.children.push_back(e);
            result.children.push_back(yes);
        })
END_RULE()

DEFINE_RULE(for_exp)
    auto f = string(); // loop variable
    auto e = ast(); // the iterator
    auto body = ast();
    // for i in e {}
    TRY(WS
        STR("for")          WS
        RULE(identifier, f) WS
        STR("in")           WS
        RULE(exp, e)        WS
        RULE(block, body)
        , {
            result.type = FOR_EXP;
            result.str_data = f;
            result.children.push_back(e);
            result.children.push_back(body);
        })
END_RULE()

DEFINE_RULE(primary_exp)
    // if expression
    auto ifexp = ast();
    TRY(WS
        RULE(if_exp, ifexp)
        , {
            result = ifexp;
        })

    // for expression
    auto forexp = ast();
    TRY(WS
        RULE(for_exp, forexp)
        , {
            result = forexp;
        })

    // range literal
    auto rexp = ast();
    TRY(WS
        RULE(range_literal, rexp)
        , {
            result = rexp;
        })

    // literal number
    auto num = 0.0;
    TRY(WS
        RULE(number, num)
        , {
            result.num_data = num;
            result.type = NUM_LITERAL;
        })

    // string literal
    auto str = string();
    TRY(WS
        RULE(string_literal, str)
        , {
            result.str_data = str;
            result.type = STRING_LITERAL;
        })

    // list literal
    auto l = ast();
    TRY(WS
        RULE(list_literal, l)
        , {
            result = l;
        })

    // literal function
    auto func = ast();
    TRY(WS
        RULE(func_literal, func)
        , {
            result = func;
        })

    // sub-expression
    auto expr = ast();
    TRY(WS
        CHAR('(')       WS
        RULE(exp, expr) WS
        CHAR(')')
        , {
            result = expr;
        })

    // actual identifier
    auto n = string();
    TRY(WS
        RULE(identifier, n)
        , {
            result.str_data = n;
            result.type = IDENTIFIER;
        })
END_RULE()

DEFINE_RULE(calling)

    // empty call variant
    TRY(WS CHAR('(') WS CHAR(')')
        , {})

    // parameter variant; append parameter values to end
    auto r = ctx.at;
    if (parse_ws(ctx) && parse_char(ctx, '(')) {
        while (true) {
            auto p = ast();
            if (parse_exp(ctx, p)) {
                result.children.push_back(p);
                parse_ws(ctx);
                if (parse_char(ctx, ',')) {
                    continue;
                } else if (parse_char(ctx, ')')) {
                    return true;
                } else {
                    ctx.at = r;
                    return false;
                }
            } else {
                ctx.at = r;
                return false;
            }
        }
    } else {
        ctx.at = r;
        return false;
    }

END_RULE()

DEFINE_RULE(indexing)
    auto e = ast();
    TRY(WS
        CHAR('[')               WS
        RULE(primary_exp, e)    WS
        CHAR(']')
        , {
            result.children.push_back(e);
        })
END_RULE()

DEFINE_RULE(postfix_exp)
    auto r = ctx.at;
    try {
        auto pexp = ast();
        if (parse_primary_exp(ctx, pexp)) {
            while (true) {
                if (parse_calling(ctx, result)) {
                    result.type = CALLING;
                    result.children.insert(result.children.begin(), pexp);
                } else if (parse_indexing(ctx, result)) {
                    result.type = INDEXING;
                    result.children.insert(result.children.begin(), pexp);
                } else {
                    result = pexp;
                }
                return true;
            }
        } else {
            ctx.at = r;
            return false;
        }
    } catch (parse_end&) {
        ctx.at = r;
        return false;
    }
END_RULE()

DEFINE_RULE(unary_exp)
    auto op = ast_operator();
    auto expr = ast();

    TRY(WS
        RULE(unary_op, op)    WS
        RULE(unary_exp, expr)
        , {
            result.children.push_back(expr);
            result.type = UNARY_EXP;
            result.op = op;
        })

    TRY(WS
        RULE(postfix_exp, result)
        , {})
END_RULE()

DEFINE_RULE(binary_exp)
    auto lhs = ast();
    auto op = ast_operator();
    auto rhs = ast();

    TRY(WS
        RULE(unary_exp, lhs)  WS
        RULE(bin_op, op)      WS
        RULE(binary_exp, rhs)
        , {
            result.children.push_back(lhs);
            result.children.push_back(rhs);
            result.type = BINARY_EXP;
            result.op = op;
        })

    TRY(WS
        RULE(unary_exp, result)
        , {})
END_RULE()

DEFINE_RULE(block)

    // empty variant
    TRY(WS STR("{") WS STR("}")
        , {})

    // populated variant
    auto restore = ctx.at;
    if (parse_ws(ctx) && parse_string(ctx, "{")) {
        while (true) {
            auto a = ast();
            if (parse_program_part(ctx, a)) {
                result.children.push_back(a);
                if (parse_terminator(ctx)) {
                    if (parse_ws(ctx) && parse_string(ctx, "}")) {
                        return true;
                    } else {
                        continue;
                    }
                }
                else {
                    parse_ws(ctx);
                    if (parse_string(ctx, "}")) {
                        return true;
                    } else {
                        ctx.at = restore;
                        return false;
                    }
                }
            } else {
                ctx.at = restore;
                return false;
            }
        }
    }
END_RULE()

DEFINE_RULE(exp)
    TRY(WS
        RULE(block, result)
        , {})
    TRY(WS
        RULE(binary_exp, result)
        , {})
END_RULE()

DEFINE_RULE(assignment)
    auto id = string();
    auto e = ast();
    auto op = ast_operator();
    TRY(WS
        RULE(identifier, id)    WS
        RULE(assign_op, op)     WS
        RULE(exp, e)
        , {
            auto identifier = ast();
            identifier.str_data = id;
            identifier.type = IDENTIFIER;

            result.type = ASSIGNMENT;
            result.op = op;
            result.children.push_back(identifier);
            result.children.push_back(e);
        })
END_RULE()

DEFINE_RULE(declaration)
    auto id = string();
    auto e = ast();
    auto op = ast_operator();
    TRY(WS
        STR("let")              WS
        RULE(identifier, id)    WS
        RULE(declare_op, op)    WS
        RULE(exp, e)
        , {
            auto identifier = ast();
            identifier.str_data = id;
            identifier.type = IDENTIFIER;

            result.type = DECLARATION;
            result.op = op;
            result.children.push_back(identifier);
            result.children.push_back(e);
        })
END_RULE()

DEFINE_RULE(program_part)
    TRY(RULE(declaration, result), {})
    TRY(RULE(assignment, result), {})
    TRY(RULE(exp, result), {})
END_RULE()

DEFINE_RULE(program)
    while (ctx.at < ctx.end) {
        auto restore = ctx.at;
        try {
            auto r = ast();
            if (parse_program_part(ctx, r) && parse_terminator(ctx)) {
                result.children.push_back(r);
                continue;
            } else {
                ctx.at = restore;
            }
        }
        catch (parse_end&) {
            ctx.at = restore;
        }

        return false;
    }
    return true;
END_RULE()

void parse(const string& data, ast& result) {
    auto ctx = parse_context(data);
    parse_program(ctx, result);
    if (ctx.at != ctx.end) {
        throw invalid_program();
    }
}

#endif
