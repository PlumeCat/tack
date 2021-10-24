#ifndef _PARSE_EXPR_H
#define _PARSE_EXPR_H

bool parse_expr(string_view& s, Ast& out_expr);
bool parse_primary_expr(string_view& s, Ast& out_expr);
bool parse_call_expr(string_view& s, Ast& out_expr);
bool parse_cmp_expr(string_view& s, Ast& out_expr);

#define parse_binary_expr(name, lhs_type, rhs_type, symbol, type) \
bool parse_##name(string_view& s, Ast& out_expr) {\
    BEGIN_PARSER();\
    auto lhs = Ast {};\
    if (parse_##lhs_type(s, lhs)) {\
        const auto s1 = s;\
        auto rhs = Ast {};\
        if (WS && EXPECT(symbol) && parse_##rhs_type(s, rhs)) {\
            out_expr = { type, ""s, 0.0, { lhs, rhs } };\
            return true;\
        }\
        s = s1;\
        out_expr = lhs;\
        return true;\
    }\
    s = _s;\
    return false;\
}

#define parse_unary_expr(name, precede, symbol, type) \
bool parse_##name(string_view& s, Ast& out_expr) {\
    BEGIN_PARSER();\
    auto subexpr = Ast {};\
    if (WS && EXPECT(symbol) && parse_##name(s, subexpr)) {\
        out_expr = { type, ""s, 0.0, { subexpr }};\
        return true;\
    }\
    s = _s;\
    return parse_##precede(s, out_expr);\
}

parse_unary_expr(un_b_not_expr, primary_expr,  '~', Ast::UnBNotExpr)
parse_unary_expr(un_not_expr,   un_b_not_expr, '!', Ast::UnNotExpr)
parse_unary_expr(un_sub_expr,   un_not_expr,   '-', Ast::UnSubExpr)
parse_unary_expr(un_add_expr,   un_sub_expr,   '+', Ast::UnAddExpr)

// TODO: figure out associativity / recursivity for most
// a - b - c is a - (b - c) or (a - b) - c ?
// or maybe refuse to compile that? is bad code style IMO
// eg lsh needs to be l-associative, but this will require shenanigans
// as the naive approach is l-recursive
parse_binary_expr(b_xor_expr, un_add_expr, b_xor_expr,'^',  Ast::BXorExpr)
parse_binary_expr(b_and_expr, b_xor_expr,  b_and_expr,'&',  Ast::BAndExpr)
parse_binary_expr(b_or_expr,  b_and_expr,  b_or_expr, '|',  Ast::BOrExpr)
parse_binary_expr(rsh_expr,   b_or_expr,   rsh_expr,  ">>", Ast::RshExpr)
parse_binary_expr(lsh_expr,   rsh_expr,    lsh_expr,  "<<", Ast::LshExpr)
parse_binary_expr(pow_expr,   lsh_expr,    pow_expr,  "**", Ast::PowExpr)
parse_binary_expr(div_expr,   pow_expr,    pow_expr,  "/",  Ast::DivExpr)
parse_binary_expr(mul_expr,   div_expr,    mul_expr,  '*',  Ast::MulExpr)
parse_binary_expr(mod_expr,   mul_expr,    mod_expr,  '%',  Ast::ModExpr)
parse_binary_expr(sub_expr,   mod_expr,    sub_expr,  '-',  Ast::SubExpr)
parse_binary_expr(add_expr,   sub_expr,    add_expr,  '+',  Ast::AddExpr)

// parse_binary_expr(cmp_lt_expr, add_expr, add_expr, '<', Ast::CmpLtExpr) // none of the cmps associate at all
// parse_binary_expr(cmp_gt_expr, add_expr, add_expr, '<', Ast::CmpGtExpr)
// parse_binary_expr(cmp_le_expr, add_expr, add_expr, "<=", Ast::CmpLeExpr)
// parse_binary_expr(cmp_ge_expr, add_expr, add_expr, ">=", Ast::CmpGeExpr)
// parse_binary_expr(cmp_ne_expr, add_expr, add_expr, "!=", Ast::CmpNeExpr)
// parse_binary_expr(cmp_eq_expr, add_expr, add_expr, "==", Ast::CmpEqExpr)
parse_binary_expr(and_expr,   cmp_expr, and_expr, "&&", Ast::AndExpr)
parse_binary_expr(xor_expr,   and_expr, xor_expr, "^^", Ast::XorExpr)
parse_binary_expr(or_expr,    xor_expr, or_expr,  "||", Ast::OrExpr)

bool parse_expr(string_view& s, Ast& out_expr) {
    return parse_or_expr(s, out_expr);
}


bool parse_primary_expr(string_view& s, Ast& out_expr) {
    BEGIN_PARSER()
    auto d = 0.0;
    auto ident = ""s;

    // number literal
    if (expect_number(s, d)) {
        out_expr = { Ast::NumLiteral, ""s, d, {} };
        return true;
    }
    s = _s;

    // call expr
    // must be here because it also starts with an identifier
    if (parse_call_expr(s, out_expr)) {
        return true;
    }
    s = _s;

    // identifier - read a variable
    if (expect_ident(s, ident)) {
        out_expr = { Ast::Identifier, ident, 0.0, {} };
        return true;
    }
    s = _s;

    // parenthesised nested expression
    if (EXPECT('(') && parse_expr(s, out_expr) && EXPECT(')')) {
        return true;
    }
    s = _s;
    
    // failed
    return false;
}
bool parse_cmp_expr(string_view& s, Ast& out_expr) {
    // have to specialise cmp expressions
    BEGIN_PARSER()

    auto lhs = Ast {};
    if (parse_add_expr(s, lhs)) {
        WS;
        // try rhs
        const auto s1 = s;
        auto rhs = Ast {};
        if (EXPECT("==") && parse_add_expr(s, rhs)) {
            out_expr = { Ast::CmpEqExpr, ""s, 0.0, { lhs, rhs }};
            return true;
        } else if (EXPECT("!=") && parse_add_expr(s, rhs)) {
            out_expr = { Ast::CmpNeExpr, ""s, 0.0, { lhs, rhs }};
            return true;
        } else if (EXPECT(">=") && parse_add_expr(s, rhs)) {
            out_expr = { Ast::CmpGeExpr, ""s, 0.0, { lhs, rhs }};
            return true;
        } else if (EXPECT("<=") && parse_add_expr(s, rhs)) {
            out_expr = { Ast::CmpLeExpr, ""s, 0.0, { lhs, rhs }};
            return true;
        } else if (EXPECT('>') && parse_add_expr(s, rhs)) {
            out_expr = { Ast::CmpGtExpr, ""s, 0.0, { lhs, rhs }};
            return true;
        } else if (EXPECT('<') && parse_add_expr(s, rhs)) {
            out_expr = { Ast::CmpLtExpr, ""s, 0.0, { lhs, rhs }};
            return true;
        }
        s = s1;
        out_expr = lhs;
        return true;
    }
    // no lhs even, fail
    s = _s;
    return false;
}

bool parse_args_list(string_view& s, Ast& out_list) {
    BEGIN_PARSER()
    out_list = { Ast::ArgsList, ""s, 0.0, {}};
    auto expr = Ast {};
    if (parse_expr(s, expr) && WS) {
        out_list.children.push_back(expr);
        while (EXPECT(',') && WS && parse_expr(s, expr)) {
            out_list.children.push_back(expr);
        }
    }
    return true;
}
bool parse_call_expr(string_view& s, Ast& out_expr) {
    BEGIN_PARSER()
    auto ident = ""s;
    auto params_list = Ast {};
    if (expect_ident(s, ident) && WS && 
        EXPECT('(') && parse_args_list(s, params_list) && WS && EXPECT(')')) {
        out_expr = { Ast::CallExpr, ident, 0.0, { params_list }};
        return true;
    }
    PARSER_FAIL()
}
bool parse_index_expr(string_view& s, Ast& out_expr) {
    BEGIN_PARSER()
    PARSER_FAIL()
}
bool parse_deref_expr(string_view& s, Ast& out_expr) {
    BEGIN_PARSER()
    PARSER_FAIL()
}



#endif
