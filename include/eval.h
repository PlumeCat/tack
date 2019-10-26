#ifndef _EVAL_H
#define _EVAL_H

double eval(const ast& ast, state& state);

double eval_un_exp(const ast& ast, state& state) {
    switch (ast.op) {
        // unary operators
        case OP_ADD: return +eval(ast.children[0], state);
        case OP_SUB: return -eval(ast.children[0], state);
        // case OP_NOT: return ~eval(ast.children[0], state);
        case OP_BOOL_NOT: return !eval(ast.children[0], state);

        default:
            throw runtime_error("Unknown operator");
    }
}

double eval_bin_exp(const ast& ast, state& state) {
    switch (ast.op) {
        // Arithmetic
        case OP_ADD:        return eval(ast.children[0], state) + eval(ast.children[1], state);
        case OP_SUB:        return eval(ast.children[0], state) - eval(ast.children[1], state);
        case OP_MUL:        return eval(ast.children[0], state) * eval(ast.children[1], state);
        case OP_DIV:        return eval(ast.children[0], state) / eval(ast.children[1], state);
        // case OP_MOD:     return eval(ast.children[0], state) % eval(ast.children[1], state);

        // Binary / bitwise
        // case OP_AND:     return eval(ast.children[0], state) & eval(ast.children[1], state);
        // case OP_OR:      return eval(ast.children[0], state) | eval(ast.children[1], state);
        // case OP_XOR:     return eval(ast.children[0], state) ^ eval(ast.children[1], state);
        case OP_BOOL_AND:   return eval(ast.children[0], state) && eval(ast.children[1], state);
        case OP_BOOL_OR:    return eval(ast.children[0], state) || eval(ast.children[1], state);

        default:
            throw runtime_error("Unknown operator");
    }
}

double eval_assignment(const ast& a, state& state) {
    assert(a.type == ASSIGNMENT);
    auto name = a.children[0].str_data;
    auto val = eval(a.children[1], state);
    state.local(name, val);
    return 1;
}
double eval_declaration(const ast& a, state& state) {
    assert(a.type == DECLARATION);
    auto name = a.children[0].str_data;
    auto val = eval(a.children[1], state);
    state.local(name, val);
    return 1;
}

double eval_program(const ast& a, state& s) {
    double res = 0.0;
    for (const auto& c: a.children) {
        res = eval(c, s);
    }
    return res;
}

double eval_calling(const ast& a, state& s) {
    // TODO: look up the function!
    if (a.children[0].str_data == "print") {
        for (int i = 1; i < a.children.size(); i++) {
            cout << eval(a.children[i], s) << " ";
        }
        cout << endl;
    } else {
        throw runtime_error("Unknown function: " + a.children[0].str_data);
    }
}

double eval(const ast& a, state& s) {
    switch (a.type) {
        // expressions
        case BINARY_EXP:    return eval_bin_exp(a, s);
        case UNARY_EXP:     return eval_un_exp(a, s);
        case LITERAL:       return a.num_data;
        case IDENTIFIER:    return s.local(a.str_data);

        // declaration and assignment
        case ASSIGNMENT:    return eval_assignment(a, s);
        case DECLARATION:   return eval_declaration(a, s);

        // calling
        case CALLING:       return eval_calling(a, s);
        
        // compound structures
        case BLOCK:
        case PROGRAM:
            return eval_program(a, s);

        // oopsies!
        default:
            throw runtime_error("Unknown AST type: ");
    }
}

#endif
