#ifndef _EVAL_H
#define _EVAL_H

value eval(const ast&, state&);

value eval_list_literal(const ast& a, state& s) {
    auto res = value(vector<value>());
    res.type = value::LIST;
    for (const auto& c: a.children) {
        res.lval.push_back(eval(c, s));
    }
    return res;
}

value eval_un_exp(const ast& a, state& s) {
    auto c = eval(a.children[0], s);
    if (c.type != value::NUMBER) {
        throw runtime_error("can't evaluate unary operator for non-number");
    }
    switch (a.op) {
        // unary operators
        case OP_ADD: return +c.dval;
        case OP_SUB: return -c.dval;
        // case OP_NOT: return ~eval(a.children[0], s);
        case OP_BOOL_NOT: return !c.dval;

        default:
            throw runtime_error("unknown operator");
    }
}

value eval_bin_exp(const ast& a, state& s) {
    auto c0 = eval(a.children[0], s);
    auto c1 = eval(a.children[1], s);
    if (c0.type != c1.type) {
        throw runtime_error("can't evaluate binary operator for mismatched types");
    }

    switch (c0.type) {
        case value::NUMBER:
            switch (a.op) {
                // Arithmetic
                case OP_ADD:        return c0.dval + c1.dval;
                case OP_SUB:        return c0.dval - c1.dval;
                case OP_MUL:        return c0.dval * c1.dval;
                case OP_DIV:        return c0.dval / c1.dval;
                // case OP_MOD:     return c0.dval % c1.dval;

                // Binary / bitwise
                // case OP_AND:     return c0.dval & c1.dval;
                // case OP_OR:      return c0.dval | c1.dval;
                // case OP_XOR:     return c0.dval ^ c1.dval;
                case OP_BOOL_AND:   return c0.dval && c1.dval;
                case OP_BOOL_OR:    return c0.dval || c1.dval;

                default:
                    throw runtime_error("invalid numerical binary operation");
            }
        case value::STRING:
            switch (a.op) {
                case OP_ADD:        return c0.sval + c1.sval;
                default:
                    throw runtime_error("invalid string binary operation");
            }
        case value::LIST:
            switch (a.op) {
                case OP_ADD: {
                    auto retval = value(c0.lval);
                    for (auto& v: c1.lval) {
                        retval.lval.push_back(v);
                    }
                    return retval;
                }
                default:
                    throw runtime_error("invalid list binary operation");
            }
    }
}

value eval_assignment(const ast& a, state& s) {
    assert(a.type == ASSIGNMENT);
    auto name = a.children[0].str_data;
    auto val = eval(a.children[1], s);
    s.set_local(name, val);
    return 1;
}
value eval_declaration(const ast& a, state& s) {
    assert(a.type == DECLARATION);
    auto name = a.children[0].str_data;
    auto val = eval(a.children[1], s);
    s.define_local(name, val);
    return 1;
}

value eval_program(const ast& a, state& s) {
    s.push_scope();
    auto res = value(0.0);
    for (const auto& c: a.children) {
        res = eval(c, s);
    }

    s.pop_scope();
    return res;
}

value eval_if_exp(const ast& a, state& s) {
    auto condition = eval(a.children[0], s);
    if (condition.type != value::NUMBER) {
        throw runtime_error("conditional expression should be a number");
    }

    if (condition.dval > 0.0) {
        return eval(a.children[1], s);
    } else {
        if (a.children.size() > 2) {
            return eval(a.children[2], s);
        } else {
            // TODO: what should if-expression return if the condition fails and there's no else?
            // Rust will give it "unit" type, whatever that means
            return value(0.0);
        }
    }
}

value eval_for_exp(const ast& a, state& s) {
    auto loop_var = a.str_data;
    auto loop_range = eval(a.children[0], s).lval;

    s.push_scope();
    s.define_local(loop_var, 0.0);
    for (auto& val : loop_range) {
        s.set_local(loop_var, val); // update loop variable
        eval(a.children[1], s);     // evaluate loop body
    }
    s.pop_scope();

    return value(0.0); // TODO: what do for loops return (if anything???)
}

value eval_indexing(const ast& a, state& s) {
    auto l = eval(a.children[0], s);
    auto i = eval(a.children[1], s);

    if (l.type != value::LIST) {
        throw runtime_error("trying to index a non-list");
    }
    if (i.type != value::NUMBER) {
        throw runtime_error("trying to index with a non-number");
    }

    return l.lval[i.dval];
}

value eval_calling(const ast& a, state& s) {
    auto fid = eval(a.children[0], s);
    if (fid.type != value::FUNCTION) {
        throw runtime_error("trying to call a non-function: "  + (string)fid);
    }

    auto func = s.functions[fid.fval.id];

    // calling scripted functions
    if (func.type == FUNC_LITERAL) {
        if (func.children.size() != a.children.size()) {
            throw runtime_error("incorrect number of arguments");
        }
        auto& body = func.children[func.children.size()-1];
        s.push_scope();

        // evaluate parameter tuple and push as local variables
        for (int i = 0; i < func.children.size()-1; i++) {
            auto argname = func.children[i].str_data;
            auto argval = eval(a.children[i+1], s);
            s.define_local(argname, argval);
        }

        // evaluate body
        auto res = eval(body, s);
        s.pop_scope();
        return res;
    } else if (func.type == NATIVE_FUNC) {
        auto nfunc = (native_function)func.ptr_data;

        // evaluate parameter tuple and put into a list
        auto args = vector<value>();
        for (int i = 0; i < a.children.size()-1; i++) {
            args.push_back(eval(a.children[i+1], s));
        }

        // evaluate native function
        auto res = nfunc(args);
        return res;
    } else {
        throw runtime_error("invalid function value");
    }

}

value eval_range_literal(const ast& a, state& s) {
    auto retval = value(vector<value>());
    auto first = eval(a.children[0], s).dval;
    auto last = eval(a.children[1], s).dval;
    for (double i = first; i < last; i++) {
        retval.lval.push_back(value(i));
    }
    return retval;
}

value eval_func_literal(const ast& a, state& s) {
    s.functions.push_back(a);
    return value(value::function { s.functions.size() - 1 });
}

value eval(const ast& a, state& s) {
    switch (a.type) {
        // expressions
        case IF_EXP:        return eval_if_exp(a, s);
        case FOR_EXP:       return eval_for_exp(a, s);
        case BINARY_EXP:    return eval_bin_exp(a, s);
        case UNARY_EXP:     return eval_un_exp(a, s);
        case LIST_LITERAL:  return eval_list_literal(a, s);
        case RANGE_LITERAL: return eval_range_literal(a, s);
        case FUNC_LITERAL:  return eval_func_literal(a, s);
        case NUM_LITERAL:   return value(a.num_data);
        case STRING_LITERAL:return value(a.str_data);
        case IDENTIFIER:    return s.get_local(a.str_data);

        // declaration and assignment
        case ASSIGNMENT:    return eval_assignment(a, s);
        case DECLARATION:   return eval_declaration(a, s);

        // calling
        case CALLING:       return eval_calling(a, s);

        // indexing
        case INDEXING:      return eval_indexing(a, s);

        // compound structures
        case BLOCK:
        case PROGRAM:
            return eval_program(a, s);

        // oopsies!
        default:
            throw runtime_error("unknown ast type: ");
    }
}

value define_function(native_function func, state& s) {
    // TODO: is this a hack?
    auto f = ast();
    f.type = NATIVE_FUNC;
    f.ptr_data = (void*)func;
    return eval_func_literal(f, s);
}

#endif
