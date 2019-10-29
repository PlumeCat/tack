#ifndef _AST_H
#define _AST_H

enum ast_type {
    PROGRAM,
    DECLARATION,
    ASSIGNMENT,
    IF_EXP,
    BINARY_EXP,
    UNARY_EXP,
    NUM_LITERAL,
    STRING_LITERAL,
    LIST_LITERAL,
    FUNC_LITERAL,
    IDENTIFIER,
    BLOCK,
    CALLING,
    INDEXING,
};

enum ast_operator {
    OP_UNKNOWN = -1,

    // Declaration
    OP_ASSIGN = 1,

    // Assignment
    OP_ASSIGN_ADD,
    OP_ASSIGN_SUB,
    OP_ASSIGN_MUL,
    OP_ASSIGN_DIV,
    OP_ASSIGN_MOD,
    OP_ASSIGN_AND,
    OP_ASSIGN_OR,
    OP_ASSIGN_XOR,
    OP_ASSIGN_LSH,
    OP_ASSIGN_RSH,

    // Comparison
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_LESS,
    OP_GREATER,
    OP_LESS_EQUAL,
    OP_GREATER_EQUAL,

    // Arithmetic
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    // Binary / bitwise
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_LSH,
    OP_RSH,
    OP_BOOL_AND,
    OP_BOOL_OR,
    OP_NOT,
    OP_BOOL_NOT,
};

string type_to_string(ast_type t) {
    const auto m = map<ast_type, string> {
        { PROGRAM,      "Program" },
        { DECLARATION,  "Declaration" },
        { ASSIGNMENT,   "Assignment" },
        { BINARY_EXP,   "BinaryExp" },
        { UNARY_EXP,    "UnaryExp" },
        { NUM_LITERAL,  "Literal" },
        { FUNC_LITERAL, "FuncLiteral" },
        { IDENTIFIER,   "Identifier" },
        { BLOCK,        "Block" },
        { CALLING,      "Calling" },
        { INDEXING,     "Indexing" },
    };
    auto f = m.find(t);
    if (f != m.end()) {
        return f->second;
    }
    return "<unknown>";
}
string op_to_string(ast_operator op) {
    const auto m = map<ast_operator, string> {
        // { OP_UNKNOWN,       "OpUnknown" },
        { OP_ASSIGN,        "OpAssign" },
        { OP_ASSIGN_ADD,    "OpAssignAdd" },
        { OP_ASSIGN_SUB,    "OpAssignSub" },
        { OP_ASSIGN_MUL,    "OpAssignMul" },
        { OP_ASSIGN_DIV,    "OpAssignDiv" },
        { OP_ASSIGN_MOD,    "OpAssignMod" },
        { OP_ASSIGN_AND,    "OpAssignAnd" },
        { OP_ASSIGN_OR,     "OpAssignOr" },
        { OP_ASSIGN_XOR,    "OpAssignXor" },
        { OP_ASSIGN_LSH,    "OpAssignLsh" },
        { OP_ASSIGN_RSH,    "OpAssignRsh" },
        { OP_EQUAL,         "OpEqual" },
        { OP_NOT_EQUAL,     "OpNoTequal" },
        { OP_LESS,          "OpLess" },
        { OP_GREATER,       "OpGreater" },
        { OP_LESS_EQUAL,    "OpLessEqual" },
        { OP_GREATER_EQUAL, "OpGreaterEqual" },
        { OP_ADD,           "OpAdd" },
        { OP_SUB,           "OpSub" },
        { OP_MUL,           "OpMul" },
        { OP_DIV,           "OpDiv" },
        { OP_MOD,           "OpMod" },
        { OP_AND,           "OpAnd" },
        { OP_OR,            "OpOr" },
        { OP_XOR,           "OpXor" },
        { OP_BOOL_AND,      "OpBoolAnd" },
        { OP_BOOL_OR,       "OpBoolOr" },
        { OP_NOT,           "OpNot" },
        { OP_BOOL_NOT,      "OpBoolNot" },
    };
    auto f = m.find(op);
    if (f != m.end()) {
        return f->second;
    }
    return "unknown";
}

struct ast {
    ast_type type = PROGRAM;
    ast_operator op = OP_UNKNOWN;
    string str_data;
    double num_data;
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
        o << indent << "Ast: "
            << type_to_string(a.type) << " | "
            << op_to_string(a.op) << " | "
            << a.num_data << " | " << (a.str_data.length() ? a.str_data : "");
        for (const auto& c: a.children) {
            o << "\n";
            recurse(recurse, c, indent2);
        }
    };
    output(output, a);
    return o;
}

#endif
