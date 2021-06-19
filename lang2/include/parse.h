#ifndef _PARSE_H
#define _PARSE_H


// TODO: target 50kloc parsed per second?

bool expect(string_view& s, const string_view& str) {
    if (s.size() && s.substr(0, str.size()) == str) {
        s.remove_prefix(str.size());
        return true;
    }
    return false;
}
bool expect(string_view& s, const char c) {
    if (s.size() && s[0] == c) {
        s.remove_prefix(1);
        return true;
    }
    return false;
}
bool expect_number(string_view& s, double& out_num) {
    if (!s.size()) {
        return false;
    }

    const char* start = &s[0];
    char* end = nullptr;

    out_num = strtod(start, &end);

    if (end != start) {
        s.remove_prefix(end - start);
        return true;
    }

    return false;
}
bool expect_ident(string_view& s, string& out_ident) {
    if (!s.size()) {
        return false;
    }
    const auto _s = s;
    if (isalpha(s[0]) || s[0] == '_') {
        s.remove_prefix(1);
        while (s.size() && (isalnum(s[0]) || s[0] == '_')) {
            s.remove_prefix(1);
        }
        out_ident = string(_s.substr(0, _s.size() - s.size()));
        return true;
    }
    return false;
}
bool skip_whitespace(string_view& s) {
    while (s.size() && isspace(s[0])) {
        s.remove_prefix(1);
    }
    return true;
}
#define EXPECT(x) expect(s, x)
#define WS skip_whitespace(s)

struct Ast {
    enum {
        #define AST_TYPE(x) x,
        #include "ast_type.xmacro"
    } type;
    string data;
    double value;
    vector<Ast> children;
};

#define PARSER_SUCCESS() { return true; }
#define PARSER_FAIL() { s = _s; return false; }
#define BEGIN_PARSER() \
    if (!s.size()) {\
        return false;\
    } WS; const auto _s = s;\

#include "parse_expr.h"

bool parse_block(string_view& s, Ast& out_expr);

bool parse_params_def(string_view& s, Ast& out) {
    BEGIN_PARSER()
    out = { Ast::ParamsDef, ""s, 0.0, {}};
    auto ident = ""s;
    if (expect_ident(s, ident) && WS) {
        out.children.push_back({ Ast::Identifier, ident, 0.0, {}});
        while (EXPECT(',') && WS && expect_ident(s, ident)) {
            out.children.push_back({ Ast::Identifier, ident, 0.0, {} });
        }
    }
    return true;
}
bool parse_func_def(string_view& s, Ast& out_ast) {
    BEGIN_PARSER()
    auto param_def = Ast {};
    auto block = Ast {};
    auto name = ""s;
    if (EXPECT("def") && WS && expect_ident(s, name) && WS &&
        EXPECT('(') && parse_params_def(s, param_def) && WS && EXPECT(')') && parse_block(s, block)) {
        out_ast = { Ast::FuncDef, name, 0.0, { param_def, block }};
        return true;
    }
    PARSER_FAIL()
}
bool parse_struct_def(string_view& s, Ast& def) { return false; }


bool parse_var_decl_stat(string_view& s, Ast& out_stat) {
    BEGIN_PARSER()
    auto ident = ""s;
    auto expr = Ast {};
    if (EXPECT("var") && WS && expect_ident(s, ident) && WS && EXPECT('=') && WS && parse_expr(s, expr)) {
        out_stat = { Ast::VarDeclStat, ident, 0.0, { expr }};
        return true;
    }
    PARSER_FAIL()
}
bool parse_assignment_stat(string_view& s, Ast& out_stat) {
    BEGIN_PARSER()
    auto ident = ""s;
    auto expr = Ast {};
    if (expect_ident(s, ident) && WS && EXPECT('=') && WS && parse_expr(s, expr)) {
        out_stat = { Ast::AssignmentStat, ident, 0.0, { expr }};
        return true;
    }
    PARSER_FAIL()
}
bool parse_print_stat(string_view& s, Ast& out_stat) {
    BEGIN_PARSER();
    auto ident = ""s;
    auto expr = Ast {};
    if (EXPECT("print") && WS && parse_expr(s, expr)) {
        out_stat = { Ast::PrintStat, ""s, 0.0, { expr }};
        return true;
    }
    PARSER_FAIL()
}


bool parse_if_stat(string_view& s, Ast& out_stat);
bool parse_else_stat(string_view& s, Ast& out_stat) {
    BEGIN_PARSER();
    if (EXPECT("else")) {
        WS;
        const auto _s1 = s;
        if (WS && parse_if_stat(s, out_stat) || parse_block(s, out_stat)) {
            return true;
        }
    }
    PARSER_FAIL()
}
bool parse_if_stat(string_view& s, Ast& out_stat) {
    BEGIN_PARSER();
    auto ident = ""s;
    auto expr = Ast {};
    if (EXPECT("if")) {
        auto block = Ast {};
        if (WS && EXPECT('(') && parse_expr(s, expr) && WS && EXPECT(')') && parse_block(s, block)) {
            out_stat = { Ast::IfStat, ""s, 0.0, { expr, block }};            
            auto else_block = Ast {};
            if (parse_else_stat(s, else_block)) {
                out_stat.children.push_back(else_block);
            }            
            return true;
        }
    }
    PARSER_FAIL()
}
// bool parse_return_stat(string_view& s, Ast& out_stat) {
//     BEGIN_PARSER();
//     auto expr = Ast {};
//     if (EXPECT("return") && WS && parse_expr(s, expr)) {
//         out_stat = { Ast::ReturnStat, ""s, 0.0, { expr }};
//         return true;
//     }
//     PARSER_FAIL()
// }

bool parse_stat(string_view& s, Ast& out_stat) {
    BEGIN_PARSER();
    
    // var decl stat
    // TODO: don't require semicolons
    // Not sure how to handle expressions running off the end of the line
    // We could be "greedy" and always try to parse the next character (javascript style)
    // this probably runs into the "return \n <expr>" problem
    // or we could treat \n as a terminator (python-style) and only continue within brackets / expressions
    // that would require the parser to remember whether it is inside an expression or not which is annoying
    // (so need to duplicate rules with inside-brackets and outside-brackets versions)
    if (parse_var_decl_stat(s, out_stat)/* && WS && EXPECT(';')*/) {
        return true;
    }
    if (parse_assignment_stat(s, out_stat)/* && WS && EXPECT(';')*/) {
        return true;
    }
    if (parse_print_stat(s, out_stat)/* && WS && EXPECT(';')*/) {
        return true;
    }
    if (parse_if_stat(s, out_stat)) {
        return true;
    }
    // if (parse_return_stat(s, out_stat)) {
    //     return true;
    // }

    PARSER_FAIL()
}
bool parse_stat_list(string_view& s, Ast& out_ast) {
    BEGIN_PARSER()

    // parse_stat_list can be empty, so always returns true
    out_ast = { Ast::Block, ""s, 0.0, {} };
    auto stat = Ast {};
    while (true) {
        if (parse_stat(s, stat)) {
            // found another statement
            out_ast.children.push_back(stat);
        } else {
            // no more valid statements
            break;
        }
    }

    return true;
}
bool parse_block(string_view& s, Ast& out_ast) {
    BEGIN_PARSER()
    auto ident = ""s;
    if (EXPECT('{')) {
        out_ast = { Ast::Block, ""s, 0.0, {} };
        auto stat = Ast {};
        while (parse_stat(s, stat)) {
            out_ast.children.push_back(stat);
        }

        if (WS && EXPECT('}')) {
            return true;
        }

        throw runtime_error("unexpected syntax: "s + string(s.substr(0, 10)) + ((s.size() > 10) ? "..." : ""));
    }
    PARSER_FAIL()
}




bool parse_module(string_view& s, Ast& out_ast) {
    BEGIN_PARSER()

    out_ast = { Ast::Module, "", 0.0, {} };
    auto def = Ast {};
    while (parse_func_def(s, def) || parse_struct_def(s, def)) {
        out_ast.children.push_back(def);
    }
    
    return true;
}

bool parse_source_file(string_view& s, Ast& out_ast) {
    parse_module(s, out_ast);

    // parse_stat_list(s, out_ast);
    // expect end of file
    if (s.size()) {
        cout << "unexpected syntax (expected EOF): " << s.substr(0, 10) << ((s.size() > 10) ? "..." : "") << endl;
        return false;
    }
    return true;
}

void print_ast(Ast& ast, const string& indent="") {
    static auto names = vector<string> {
        #define AST_TYPE(x) #x,
        #include "ast_type.xmacro"
    };
    cout << indent << names[ast.type] << ": " << ast.data << " " << 
        (ast.type == Ast::NumLiteral ? to_string(ast.value) : ""s) << endl;
    for (auto& a: ast.children) {
        print_ast(a, indent + "    ");
    }
}


#endif
