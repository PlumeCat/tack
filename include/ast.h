#ifndef _AST_H
#define _AST_H

#include "ast_operator.h"

enum _ast_type {
    PROGRAM,
    TYPE_DECL,
    DECLARATION,
    ASSIGNMENT,
    LOCATOR,

    IF_EXP,
    FOR_EXP,
    BLOCK,
    BINARY_EXP,
    UNARY_EXP,

    CALLING,
    INDEXING,
    GET_FIELD,

    IDENTIFIER,
    NUM_LITERAL,
    STRING_LITERAL,
    RANGE_LITERAL,
    LIST_LITERAL,
    FUNC_LITERAL,

    NATIVE_FUNC, // only used by define_function
};

string type_to_string(ast_type t) {
    const auto m = map<ast_type, string> {
        { PROGRAM,          "Program" },
        { TYPE_DECL,        "TypeDecl" },
        { DECLARATION,      "Declaration" },
        { ASSIGNMENT,       "Assignment" },
        { LOCATOR,          "Locator" },

        { IF_EXP,           "IfExp" },
        { FOR_EXP,          "ForExp" },
        { BLOCK,            "Block" },
        { BINARY_EXP,       "BinaryExp" },
        { UNARY_EXP,        "UnaryExp" },

        { CALLING,          "Calling" },
        { INDEXING,         "Indexing" },
        { GET_FIELD,        "GetField" },

        { IDENTIFIER,       "Identifier" },
        { NUM_LITERAL,      "NumLiteral" },
        { STRING_LITERAL,   "StringLiteral" },
        { LIST_LITERAL,     "ListLiteral" },
        { RANGE_LITERAL,    "RangeLiteral" },
        { FUNC_LITERAL,     "FuncLiteral" },

        { NATIVE_FUNC,      "NativeFunc" },
    };
    auto f = m.find(t);
    if (f != m.end()) {
        return f->second;
    }
    return "<unknown>";
}

enum class ast_type : unsigned int {

};

struct ast_base {};

struct ast_program : ast_base {};
struct ast_type_decl : ast_base {};

struct ast {
    ast_type type = PROGRAM;
    ast_operator op = OP_UNKNOWN;
    string str_data;
    double num_data;
    void* ptr_data = nullptr;
    vector<ast> children;

    ast() = default;
    ast(ast_type _type, ast_operator _op, string _data="") {
        type = _type;
        op = _op;
        str_data = _data;
    }
    ast(ast_type _type, ast_operator _op, double _data=0) {
        type = _type;
        op = _op;
        num_data = _data;
    }
    ast(const ast&) = default;
    ast(ast&&) = default;
    ast& operator=(const ast&) = default;
    ast& operator=(ast&&) = default;
};

ostream& operator << (ostream& o, const ast& a) {
    auto output = [&](auto recurse, const ast& a, const string& indent="") -> void {
        const auto indent2 = indent + "    ";
        o << indent //<< "Ast: "
            << type_to_string(a.type)   << ":"
            << (a.type == DECLARATION || a.type == ASSIGNMENT || a.type == BINARY_EXP || a.type == UNARY_EXP ? (op_to_string(a.op)) : "")
            << (a.type == NUM_LITERAL ? (" " + to_string(a.num_data)) : "")
            << (a.str_data.length() ? (" " + a.str_data) : "")
            ;
        for (const auto& c: a.children) {
            o << "\n";
            recurse(recurse, c, indent2);
        }
    };
    output(output, a);
    return o;
}

#endif
