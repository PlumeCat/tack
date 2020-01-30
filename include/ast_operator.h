// ast_operator.h
#ifndef _AST_OPERATOR_H
#define _AST_OPERATOR_H

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

    // Binary
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_LSH,
    OP_RSH,

    // Boolean
    OP_BOOL_AND,
    OP_BOOL_OR,

    // Unary only
    OP_NOT,
    OP_BOOL_NOT,
};

string op_to_string(ast_operator op) {
    const auto m = map<ast_operator, string> {
        // assignments
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

        // comparisons
        { OP_EQUAL,         "OpEqual" },
        { OP_NOT_EQUAL,     "OpNoTequal" },
        { OP_LESS,          "OpLess" },
        { OP_GREATER,       "OpGreater" },
        { OP_LESS_EQUAL,    "OpLessEqual" },
        { OP_GREATER_EQUAL, "OpGreaterEqual" },

        // arithmetic
        { OP_ADD,           "OpAdd" },
        { OP_SUB,           "OpSub" },
        { OP_MUL,           "OpMul" },
        { OP_DIV,           "OpDiv" },
        { OP_MOD,           "OpMod" },

        // bitwise
        { OP_AND,           "OpAnd" },
        { OP_OR,            "OpOr" },
        { OP_XOR,           "OpXor" },
        { OP_LSH,           "OpLsh" },
        { OP_RSH,           "OpRsh" },

        // boolean
        { OP_BOOL_AND,      "OpBoolAnd" },
        { OP_BOOL_OR,       "OpBoolOr" },

        // unary only
        { OP_NOT,           "OpNot" },
        { OP_BOOL_NOT,      "OpBoolNot" },
    };
    auto f = m.find(op);
    if (f != m.end()) {
        return f->second;
    }
    return "unknown";
}

#endif