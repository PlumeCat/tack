#ifndef _EVAL_H
#define _EVAL_H

double eval(const ast& ast);

double eval_un_exp(const ast& ast) {
    switch (ast.op) {
        // unary operators
        case OP_ADD: return +eval(ast.children[0]);
        case OP_SUB: return -eval(ast.children[0]);
        // case OP_NOT: return ~eval(ast.children[0]);
        case OP_BOOL_NOT: return !eval(ast.children[0]);

        default:
            throw runtime_error("Unknown operator");
    }
}

double eval_bin_exp(const ast& ast) {
    switch (ast.op) {
        // Arithmetic
        case OP_ADD:        return eval(ast.children[0]) + eval(ast.children[1]);
        case OP_SUB:        return eval(ast.children[0]) - eval(ast.children[1]);
        case OP_MUL:        return eval(ast.children[0]) * eval(ast.children[1]);
        case OP_DIV:        return eval(ast.children[0]) / eval(ast.children[1]);
        // case OP_MOD:     return eval(ast.children[0]) % eval(ast.children[1]);

        // Binary / bitwise
        // case OP_AND:     return eval(ast.children[0]) & eval(ast.children[1]);
        // case OP_OR:      return eval(ast.children[0]) | eval(ast.children[1]);
        // case OP_XOR:     return eval(ast.children[0]) ^ eval(ast.children[1]);
        case OP_BOOL_AND:   return eval(ast.children[0]) && eval(ast.children[1]);
        case OP_BOOL_OR:    return eval(ast.children[0]) || eval(ast.children[1]);

        default:
            throw runtime_error("Unknown operator");
    }
}

double eval(const ast& a) {
    switch (a.type) {
        case BINARY_EXP:    return eval_bin_exp(a);
        case UNARY_EXP:     return eval_un_exp(a);
        case LITERAL:       return a.num_data;
        case IDENTIFIER:    return 0;
        case PROGRAM: {
            double res = 0.0;
            for (const auto& c: a.children) {
                res = eval(c);
                cout << "  ephemeral result: " << res << endl;
            }
            return res;
        }
        default:
            throw runtime_error("Unknown AST type: ");
    }
}

#endif
