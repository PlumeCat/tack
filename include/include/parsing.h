#ifndef _PARSING_H
#define _PARSING_H


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

    char peek() const {
        if (at == end) {
            throw runtime_error("Unexpected end of parsing data");
        }
        return *at;
    }
    void advance(int n = 1) {
        if (end - at < n) {
            throw runtime_error("Unexpected end of parsing data");
        }
        at += n;
    }
};


//////////////////////////////////////////////////////////////////////////
// Parsing simple terminals
//////////////////////////////////////////////////////////////////////////
bool parse_name(parse_context& ctx, string& name) {
    auto start = ctx.at;
    auto f = ctx.peek();
    
    if (f == '_' ||
        (f >= 'A' && f <= 'Z') ||
        (f >= 'a' && f <= 'z')) {
        ctx.advance();
        
        while (true) {
            auto c = ctx.peek();
            if (c != '_' ||
                !(c >= 'A' && c <= 'Z') ||
                !(c >= 'a' && c <= 'z') ||
                !(c >= '0' && c <= '9')) {
                break;
            }
            ctx.advance();
        }
        
        name = string(start, ctx.at);
        return true;
    }

    return false;
}
bool parse_char(parse_context& ctx, char c) {
    if (*ctx.at == c) {
        ctx.advance();
        return true;
    }
    return false;
}
bool parse_ws(parse_context& ctx) {
    while (isspace(*ctx.at)) {
        ctx.advance();
    }
    return true;
}
template<int N>
bool parse_str(parse_context& ctx, const char (&str) [N]) {
    // TODO: dodgy address-of-reference?
    if (strncmp(&*ctx.at, str, N) == 0) {
        ctx.advance(N);
        return true;
    }
    return false;
}
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
bool parse_unary_op(parse_context& ctx, ast_operator& result) {
    result = parse_char(ctx, '+') ? OP_ADD :
            parse_char(ctx, '-') ? OP_SUB :
            parse_char(ctx, '!') ? OP_BOOL_NOT :
            parse_char(ctx, '~') ? OP_NOT :
            OP_UNKNOWN;
    return (result != OP_UNKNOWN);
}
bool parse_bin_op(parse_context& ctx, ast_operator& result) {
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

bool parse_exp(parse_context& ctx, ast& result);

#define _cat(a, b) a##b
#define cat(a, b) _cat(a, b)
#define TRY(exp, block) \
    auto cat(_restore_at, __LINE__) = ctx.at;\
    if (true exp) {\
        block\
        return true;\
    }\
    ctx.at = cat(_restore_at, __LINE__);

#define DEFINE_RULE(name) bool name(parse_context& ctx, ast& result) {
#define END_RULE()        return false; }
#define WS              && parse_ws(ctx)
#define CHAR(c)         && parse_char(ctx, c)
#define RULE(rule, res) && rule(ctx, res)


DEFINE_RULE(parse_primary_exp)
    // bracketed expression
    auto expr = ast();
    TRY(WS
        CHAR('(')               WS
        RULE(parse_exp, expr)   WS
        CHAR(')')               WS, {
            result = expr;
        }
    )

    // literal number
    auto num = 0.0;
    TRY(WS
        RULE(parse_number, num), {
            result.num_data = num;
            result.type = ast_type::LITERAL;
        }
    )

    // name
END_RULE()

DEFINE_RULE(parse_unary_exp)
    auto op = ast_operator();
    auto expr = ast();
    TRY(WS
        RULE(parse_unary_op, op) WS
        RULE(parse_unary_exp, expr), {
            result.children.push_back(expr);
            result.type = UNARY_EXP;
            result.op = op;
        })
    
    TRY(WS
        RULE(parse_primary_exp, result), {})
END_RULE()
    
//bool parse_unary_exp(parse_context& ctx, ast& result) {
//     auto orig = ptr;

//     auto op = ast_operator();
//     auto exp = ast();

//     if (parse_ws(ptr) &&
//         parse_unary_op(ptr, op) &&
//         parse_ws(ptr) &&
//         parse_unary_exp(ptr, exp)) {
//         result.children.push_back(exp);
//         result.type = ast_type::UNARY_EXP;
//         result.op = op;
//         return true;
//     }
    
//     ptr = orig;
//     return parse_primary_exp(ptr, result);
// }

DEFINE_RULE(parse_binary_exp)
    auto lhs = ast();
    auto op = ast_operator();
    auto rhs = ast();
    
    TRY(WS
        RULE(parse_unary_exp, lhs) WS
        RULE(parse_bin_op, op) WS
        RULE(parse_binary_exp, rhs)
        , {
            result.children.push_back(lhs);
            result.children.push_back(rhs);
            result.type = BINARY_EXP;
            result.op = op;
        })
    
    TRY(WS
        RULE(parse_unary_exp, result), {})
END_RULE()

// bool parse_binary_exp(parse_context& ctx, ast& result) {
//     auto orig = ptr;

//     auto lhs = ast();
//     auto op = ast_operator();
//     auto rhs = ast();
//     if (parse_unary_exp(ptr, lhs) &&
//         parse_ws(ptr) &&
//         parse_bin_op(ptr, op) &&
//         parse_ws(ptr) &&
//         parse_binary_exp(ptr, rhs)) {
        
//         result.children.push_back(lhs);
//         result.children.push_back(rhs);
//         result.op = op;
//         result.type = ast_type::BINARY_EXP;
//         return true;
//     }
    
//     ptr = orig;
//     return parse_unary_exp(ptr, result); // TODO: already did this above, optimize
// }


bool parse_exp(parse_context& ctx, ast& result) {
    return parse_binary_exp(ctx, result);
}

bool parse_program(const string& data, ast& result) {
    auto ctx = parse_context(data);
    return parse_binary_exp(ctx, result);
}

#endif