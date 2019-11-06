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
DECLARE_RULE(name) {
#define TRY2(exp, stuff, line) \
    auto cat(_restore, line) = ctx.at;\
    try {\
        if (true exp) { \
            stuff\
            return true; \
        }\
        ctx.at = cat(_restore, line);\
    } catch (parse_end&) {\
        ctx.at = cat(_restore, line);\
    }
#define END_RULE()\
    return false;\
}
#define TRY(exp, stuff) TRY2(exp, stuff, __LINE__)

// parsing DSL "combinators"
#define WS                  && parse_ws(ctx)
#define CHAR(c)             && parse_char(ctx, c)
#define STR(s)              && parse_string(ctx, s)
#define RULE(rule, res)     && rulename(rule)(ctx, res)
#define OPT(rule)           && ((true rule) || true)
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
    result =parse_char(ctx, '+') ? OP_ADD :
            parse_char(ctx, '-') ? OP_SUB :
            parse_char(ctx, '!') ? OP_BOOL_NOT :
            parse_char(ctx, '~') ? OP_NOT :
            OP_UNKNOWN;
    return (result != OP_UNKNOWN);
}

// consume a comparison operator
// return false if not found
// throw if EOF
bool parse_cmp_op(parse_context& ctx, ast_operator& result) {
    // no need to catch EOF; if one throws it they all would throw it
    // just make sure to try them in reverse order of length to preserve this
    result =parse_char(ctx, '<') ? OP_LESS :
            parse_char(ctx, '>') ? OP_GREATER:
            parse_string(ctx, "==") ? OP_MUL :
            parse_string(ctx, "!=") ? OP_DIV :
            parse_string(ctx, "<=") ? OP_MOD :
            parse_string(ctx, ">=") ? OP_MOD :
            OP_UNKNOWN;
    return result != OP_UNKNOWN;
}

bool parse_bin_op(parse_context& ctx, ast_operator& result) {
    result =parse_char(ctx, '+') ? OP_ADD :
            parse_char(ctx, '-') ? OP_SUB :
            parse_char(ctx, '*') ? OP_MUL :
            parse_char(ctx, '/') ? OP_DIV :
            parse_char(ctx, '%') ? OP_MOD :
            parse_char(ctx, '&') ? OP_AND :
            parse_char(ctx, '|') ? OP_OR :
            parse_char(ctx, '^') ? OP_XOR :
            parse_string(ctx, "<<") ? OP_LSH :
            parse_string(ctx, ">>") ? OP_RSH :
            parse_string(ctx, "&&") ? OP_BOOL_AND :
            parse_string(ctx, "||") ? OP_BOOL_OR :
            OP_UNKNOWN;
    return result != OP_UNKNOWN;
}


//////////////////////////////////////////////////////////////////////////
// Parsing expressions
//////////////////////////////////////////////////////////////////////////

// forward declaration
DECLARE_RULE(exp);
DECLARE_RULE(primary_exp);
DECLARE_RULE(indexing);
DECLARE_RULE(program_part);
DECLARE_RULE(block);
DECLARE_RULE(func_body);


DEFINE_RULE(param_list)
    auto i = ast(); i.type = IDENTIFIER;
    auto j = ast();

    TRY(WS
        RULE(identifier, i.str_data)    WS
        CHAR(',')                       WS
        RULE(param_list, result)
        , {
            result.children.insert(result.children.begin(), i);
        })

    TRY(WS
        RULE(identifier, i.str_data)
        , {
            result.children.push_back(i);
        })
END_RULE()

DEFINE_RULE(func_literal)
    auto body = ast();
    TRY(WS
        CHAR('(')               WS
        CHAR(')')               WS
        STR("=>")               WS
        RULE(func_body, body)
        , {
            result.type = FUNC_LITERAL;
            result.children.push_back(body);
        })
    auto params = ast();
    body = ast();
    TRY(WS
        CHAR('(')                   WS
        RULE(param_list, params)    WS
        CHAR(')')                   WS
        STR("=>")                   WS
        RULE(func_body, body)
        , {
            result.type = FUNC_LITERAL;
            result.children = params.children; // overwrite
            result.children.push_back(body);
        })
END_RULE()

DEFINE_RULE(list_elements)
    auto e = ast();
    TRY(WS
        RULE(exp, e)                WS
        CHAR(',')                   WS
        RULE(list_elements,result)
        , {
            result.children.insert(result.children.begin(), e);
        })

    e = ast();
    TRY(WS
        RULE(exp, e)
        , {
            result.children.push_back(e);
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
    TRY(WS
        CHAR('[')                   WS
        RULE(list_elements, result) WS
        CHAR(']')
        , {
            result.type = LIST_LITERAL;
        })
END_RULE()

DEFINE_RULE(argument_list)
    auto e = ast();
    TRY(WS
        RULE(exp, e)    WS
        CHAR(',')       WS
        RULE(argument_list, result)
        , {
            result.children.insert(result.children.begin(), e);
        })

    e = ast();
    TRY(WS
        RULE(exp, e)
        , {
            result.children.push_back(e);
        })
END_RULE()

DEFINE_RULE(calling)
    TRY(WS
        CHAR('(')                   WS
        RULE(argument_list, result) WS
        CHAR(')')
        , {
            result.type = CALLING;
        })
    TRY(WS
        CHAR('(') WS
        CHAR(')')
        , {
            result.type = CALLING;
        })
END_RULE()

DEFINE_RULE(indexing)
    auto pexp = ast();
    TRY(WS
        CHAR('[')           WS
        RULE(exp, pexp)   WS
        CHAR(']')
        , {
            result.type = INDEXING;
            result.children.push_back(pexp);
        })
END_RULE()

DEFINE_RULE(postfix_exp)
    auto pexp = ast();
    TRY(WS
        RULE(primary_exp, pexp)
        , {
            // Thus far the only left-recursive rule...
            while (true) {
                auto call = ast();
                auto index = ast();
                if (parse_calling(ctx, call)) {
                    call.children.insert(call.children.begin(), pexp);
                    pexp = call;
                } else if (parse_indexing(ctx, index)) {
                    index.children.insert(index.children.begin(), pexp);
                    pexp = index;
                } else {
                    result = pexp;
                    return true;
                }
            }
        })
END_RULE()

DEFINE_RULE(block_contents)
    auto p = ast();
    TRY(WS
        RULE(program_part, p)           TERM
        RULE(block_contents, result)
        , {
            result.children.insert(result.children.begin(), p);
        })

    p = ast();
    TRY(WS
        RULE(program_part, p)
        OPT(TERM)
        WS
        , {
            result.children.push_back(p);
        })
END_RULE()

DEFINE_RULE(block)
    TRY(WS
        CHAR('{')    WS
        CHAR('}')
        , {})

    TRY(WS
        CHAR('{')                       WS
        RULE(block_contents, result)    WS
        CHAR('}')
        , {})
END_RULE()

DEFINE_RULE(func_body)
    TRY(WS
        RULE(exp, result)
        , {})
END_RULE()

DEFINE_RULE(range_literal)
    auto n1 = 0.0;
    auto n2 = 0.0;

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
    // block
    auto blk = ast();
    TRY(WS
        RULE(block, blk)
        , {
            result = blk;
        })

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

    // function literal
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

    // #define DEF_BIN_OP(_op, _opstring, _precede)\
    //     DEFINE_RULE(_op)\
    //         auto lhs = ast();\
    //         auto rhs = ast();\
    //         TRY2(WS\
    //             RULE(_precede, lhs)  WS\
    //             STR(_opstring)       WS\
    //             RULE(_op, rhs)\
    //             , {\
    //                 result.type = BINARY_EXP;\
    //                 result.children.push_back(lhs);\
    //                 result.children.push_back(rhs);\
    //                 result.op = cat(OP_, _op);\
    //             }, 1)\
    //         TRY2(WS\
    //             RULE(_precede, result)\
    //             , {\
    //             }, 2)\
    //     END_RULE()

    // DEF_BIN_OP(MOD,         "%",   unary_exp)
    // DEF_BIN_OP(DIV,         "/",   MOD)
    // DEF_BIN_OP(MUL,         "*",   DIV)
    // DEF_BIN_OP(SUB,         "-",   MUL)
    // DEF_BIN_OP(ADD,         "+",   SUB)

    // DEF_BIN_OP(RSH,         ">>",   ADD)
    // DEF_BIN_OP(LSH,         "<<",   RSH)
    // DEF_BIN_OP(XOR,         "^",    LSH)
    // DEF_BIN_OP(AND,         "&",    XOR)
    // DEF_BIN_OP(OR,          "|",    AND)

    // DECLARE_RULE(comparison);

    // DEF_BIN_OP(BOOL_AND,    "&&",   comparison)
    // DEF_BIN_OP(BOOL_OR,     "||",   BOOL_AND)

    // DEFINE_RULE(comparison)
    //     auto lhs = ast();
    //     auto rhs = ast();
    //     auto op = ast_operator();
    //     TRY(WS
    //         RULE(OR, lhs)       WS
    //         RULE(cmp_op, op)    WS
    //         RULE(OR, rhs)
    //         , {
    //             result.children.push_back(lhs);
    //             result.children.push_back(rhs);
    //             result.type = BINARY_EXP;
    //             result.op = op;
    //         })
    //     TRY(WS
    //         RULE(OR, result)
    //         , {})
    // END_RULE()

DEFINE_RULE(exp)
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

void parse(const string& data, ast& result) {
    auto ctx = parse_context(data);
    if (!parse_block_contents(ctx, result)) {
        throw invalid_program();
    }
    if (ctx.at != ctx.end) {
        throw invalid_program();
    }
}

#endif
