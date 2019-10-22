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


// Parsing DSL (TODO: needs work)
#define _cat(a, b) a##b
#define cat(a, b) _cat(a, b)
#define TRY2(exp, block) \
    auto cat(_restore_at, __LINE__) = ctx.at;\
    if (true exp) {\
        block\
        return true;\
    }\
    ctx.at = cat(_restore_at, __LINE__);

#define rulename(rule) cat(parse_, rule)


// begin parsing function
#define DEFINE_RULE(name)\
bool rulename(name)(parse_context& ctx, ast& result) {\
    auto _eof = false;
#define TRY(exp, block) \
    auto cat(_restore, __LINE__) = ctx.at;\
    try {\
        if (true exp) { \
            block\
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

// parsing "combinators"
#define WS                  && parse_ws(ctx)
#define CHAR(c)             && parse_char(ctx, c)
#define RULE(rule, res)     && rulename(rule)(ctx, res)
#define TERM                && parse_terminator(ctx)



//////////////////////////////////////////////////////////////////////////
// Parsing simple terminals
//////////////////////////////////////////////////////////////////////////


// consume valid identifier
// return false if identifier not found
// throw if EOF
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

// consume the statement / expression terminator
// return true if found
// can be ';', '\n' or EOF (ergo, doesn't throw on EOF)
bool parse_terminator(parse_context& ctx) {
    try {
        return parse_char(ctx, ';');
    } catch (parse_end&) {
        return true;
    }
    return false;
    // try {
    //     auto c = ctx.peek();
    //     if (c == ';' || c == '\n' || c == '\r') {
    //         ctx.advance();
    //         return true;
    //     }
    // } catch (parse_end&) {
    //     return true;
    // }

    // return false;
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
// return false if not found
// throw if EOF
// template<int N>
// bool parse_str(parse_context& ctx, const char (&str) [N]) {
//     // TODO: dodgy address-of-reference?
//     if (strncmp(&*ctx.at, str, N) == 0) {
//         ctx.advance(N);
//         return true;
//     }
//     return false;
// }

// consume a number (and leading whitespace)
// return false if not found
// throw if EOF
bool parse_number(parse_context& ctx, double& num) {
    // TODO: dodgy address-of-reference?
    const auto start_pos = &*ctx.at;
    auto end_pos = (char*)nullptr;
    num = strtof(start_pos, &end_pos);
    if (end_pos == start_pos) {
        return false;
    }
    ctx.advance(end_pos - start_pos);
    return true;
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
bool parse_exp(parse_context& ctx, ast& result);



DEFINE_RULE(primary_exp)
    // bracketed expression
    auto expr = ast();
    TRY(WS
        CHAR('(')       WS
        RULE(exp, expr) WS
        CHAR(')')       WS
        , {
            result = expr;
        }
    )

    // literal number
    auto num = 0.0;
    TRY(WS RULE(number, num) WS
        , {
            result.num_data = num;
            result.type = ast_type::LITERAL;
        }
    )

    // name
    auto n = string();
    TRY(WS RULE(identifier, n) WS
        , {
            result.str_data = n;
            result.type = ast_type::IDENTIFIER;
        }
    )
END_RULE()



DEFINE_RULE(unary_exp)
    auto op = ast_operator();
    auto expr = ast();
    
    TRY(WS
        RULE(unary_op, op)    WS
        RULE(unary_exp, expr) WS
        , {
            result.children.push_back(expr);
            result.type = UNARY_EXP;
            result.op = op;
        })

    TRY(WS RULE(primary_exp, result) WS
        , {})
END_RULE()



DEFINE_RULE(binary_exp)
    auto lhs = ast();
    auto op = ast_operator();
    auto rhs = ast();

    TRY(WS
        RULE(unary_exp, lhs)  WS
        RULE(bin_op, op)      WS
        RULE(binary_exp, rhs) WS
        , {
            result.children.push_back(lhs);
            result.children.push_back(rhs);
            result.type = BINARY_EXP;
            result.op = op;
        })

    TRY(WS RULE(unary_exp, result) WS
        , {})
END_RULE()


DEFINE_RULE(exp)
    TRY(WS RULE(binary_exp, result) TERM WS
        , {})
END_RULE()


void parse_program(const string& data, ast& result) {
    auto ctx = parse_context(data);

    while (true) {
        auto e = ast();
        //cout << " -- remaining: {{" << string(ctx.at, ctx.end) << "}}" << endl;

        if (!parse_exp(ctx, e)) {
            throw invalid_program();
        }
        
        result.children.push_back(e);

        if (ctx.at == ctx.end) {
            break;
        }
    }
}

#endif
