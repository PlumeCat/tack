#ifndef _EVAL_H
#define _EVAL_H

#include <cmath>

struct Value {
    using number_type = double;
    using string_type = string;
    enum Type {
        Bool,
        Number,
        String,
    };
    bool bool_val;
    number_type num_val;
    string_type str_val;
    Type type;

    Value() { type = Number; num_val = 0.0; }
    Value(bool b) { type = Bool; bool_val = b; }
    Value(number_type n) { type = Number; num_val = n; }
    Value(const string_type& s) { type = String; str_val = s; }
    
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;
    Value(const Value&) = default;
    Value(Value&&) = default;
    
    string str() {
        switch (type) {
            case Number: return to_string(num_val);
            case String: return '"' + str_val + '"';
            case Bool: return bool_val ? "true" : "false";
            default: return "<unknown>";
        }
    }
};
struct VM {
    struct Scope : public unordered_map<string, double> { bool lexical_barrier = false; }; // TODO: lexical_barrier is probably the wrong name for this
    vector<Scope> scopes;
    unordered_map<string, Ast> functions;

    Scope& cur_scope() {
        return scopes[scopes.size()-1];
    }

    void add_var(const string& name, double val) {
        if (cur_scope().find(name) != cur_scope().end()) {
            throw runtime_error("can't declare variable; '" + name + "' already exists in current scope");
        }
        cur_scope()[name] = val;
    }
    double get_var(const string& name) {
        for (auto s = scopes.rbegin(); s != scopes.rend(); s++) {
            auto var = s->find(name);
            if (var != s->end()) {
                return var->second; // found the variable
            }
            if (s->lexical_barrier) {
                break; // don't search above lexical barriers
            }
        }
        throw runtime_error("can't find variable '" + name + "' in current scope or any accessible parent scope");
    }
    void set_var(const string& name, double val) {
        for (auto s = scopes.rbegin(); s != scopes.rend(); s++) {
            auto var = s->find(name);
            if (var != s->end()) {
                var->second = val;
                return;
            }
            if (s->lexical_barrier) {
                break;
            }
        }
        throw runtime_error("can't assign variable; '" + name + "' does not exist in current scope");
    }
    void push_scope(bool lexical_barrier=false) {
        scopes.push_back(Scope {});
        scopes.back().lexical_barrier = lexical_barrier;
    }
    void pop_scope() {
        scopes.pop_back();
    }
};

double eval_expr(Ast& ast, VM& vm);
void eval_block(Ast& ast, VM& vm);

double eval_call_expr(Ast& ast, VM& vm) {
    auto func = vm.functions.find(ast.data);
    if (func == vm.functions.end()) {
        throw runtime_error("no such function 'func'");
    }

    vm.push_scope(true);

    // set arguments
    auto& args = ast.children[0].children;
    auto& params = func->second.children[0].children;
    if (params.size() != args.size()) {
        throw runtime_error("function '" + func->second.data + "' expected " + to_string(params.size()) + " arguments, received " + to_string(args.size()));
    }
    for (int i = 0; i < params.size(); i++) {
        auto val = eval_expr(args[i], vm);
        auto& name = params[i].data;
        vm.add_var(name, val);
    }

    // evaluate function
    eval_block(func->second.children[1], vm);

    vm.pop_scope();
    return 0.0; // TODO: return value
}

double eval_expr(Ast& ast, VM& vm) {
    switch (ast.type) {
        case Ast::AndExpr:      return (bool)eval_expr(ast.children[0], vm) && (bool)eval_expr(ast.children[1], vm);
        case Ast::OrExpr:       return (bool)eval_expr(ast.children[0], vm) || (bool)eval_expr(ast.children[1], vm);
        case Ast::XorExpr:      return (bool)eval_expr(ast.children[0], vm) ^ (bool)eval_expr(ast.children[1], vm);
        
        case Ast::CmpEqExpr:    return eval_expr(ast.children[0], vm) == eval_expr(ast.children[1], vm);
        case Ast::CmpNeExpr:    return eval_expr(ast.children[0], vm) != eval_expr(ast.children[1], vm);
        case Ast::CmpGeExpr:    return eval_expr(ast.children[0], vm) >= eval_expr(ast.children[1], vm);
        case Ast::CmpLeExpr:    return eval_expr(ast.children[0], vm) <= eval_expr(ast.children[1], vm);
        case Ast::CmpGtExpr:    return eval_expr(ast.children[0], vm) > eval_expr(ast.children[1], vm);
        case Ast::CmpLtExpr:    return eval_expr(ast.children[0], vm) < eval_expr(ast.children[1], vm);
        
        case Ast::AddExpr:      return eval_expr(ast.children[0], vm) + eval_expr(ast.children[1], vm);
        case Ast::SubExpr:      return eval_expr(ast.children[0], vm) - eval_expr(ast.children[1], vm);
        case Ast::ModExpr:      return (int)eval_expr(ast.children[0], vm) % (int)eval_expr(ast.children[1], vm);
        case Ast::MulExpr:      return eval_expr(ast.children[0], vm) * eval_expr(ast.children[1], vm);
        case Ast::DivExpr:      return eval_expr(ast.children[0], vm) / eval_expr(ast.children[1], vm);
        case Ast::PowExpr:      return pow(eval_expr(ast.children[0], vm), eval_expr(ast.children[1], vm));
        case Ast::LshExpr:      return (int)eval_expr(ast.children[0], vm) << (int)eval_expr(ast.children[1], vm);
        case Ast::RshExpr:      return (int)eval_expr(ast.children[0], vm) >> (int)eval_expr(ast.children[1], vm);
        
        case Ast::BAndExpr:     return (int)eval_expr(ast.children[0], vm) & (int)eval_expr(ast.children[1], vm);
        case Ast::BOrExpr:      return (int)eval_expr(ast.children[0], vm) | (int)eval_expr(ast.children[1], vm);
        case Ast::BXorExpr:     return (int)eval_expr(ast.children[0], vm) ^ (int)eval_expr(ast.children[1], vm);
        
        case Ast::UnNotExpr:    return !(bool)eval_expr(ast.children[0], vm);
        case Ast::UnBNotExpr:   return ~(int)eval_expr(ast.children[0], vm);
        case Ast::UnAddExpr:    return +eval_expr(ast.children[0], vm);
        case Ast::UnSubExpr:    return -eval_expr(ast.children[0], vm);

        case Ast::CallExpr:     return eval_call_expr(ast, vm);
        
        case Ast::NumLiteral:   return ast.value;
        case Ast::Identifier:   return vm.get_var(ast.data);
        case Ast::Unknown:      throw runtime_error("unknown ast type");
    }
}

void eval_var_decl(Ast& ast, VM& vm) {
    vm.add_var(ast.data, eval_expr(ast.children[0], vm));
}
void eval_assignment(Ast& ast, VM& vm) {
    vm.set_var(ast.data, eval_expr(ast.children[0], vm));
}
void eval_print_stat(Ast& ast, VM& vm) {
    cout << " > " << eval_expr(ast.children[0], vm) << endl;
}
void eval_expr_stat(Ast& ast, VM& vm) {
    eval_expr(ast.children[0], vm);
}

void eval_if_stat(Ast& ast, VM& vm) {
    auto cond = eval_expr(ast.children[0], vm);
    if (cond) {
        eval_block(ast.children[1], vm);
    } else {
        if (ast.children.size() > 2) {
            switch (ast.children[2].type) {
                case Ast::IfStat: eval_if_stat(ast.children[2], vm); break;
                case Ast::Block: eval_block(ast.children[2], vm); break;
            }
        }
    }
}

void eval_func_def(Ast& ast, VM& vm) {
    if (vm.functions.find(ast.data) != vm.functions.end()) {
        throw runtime_error("function '" + ast.data + "' already exists");
    }
    vm.functions[ast.data] = ast;
}
void eval_struct_def(Ast& ast, VM& vm) {
    // TODO!
    throw runtime_error("structs are not yet implemented!");
}

void eval_block(Ast& ast, VM& vm) {
    vm.push_scope();
    for (auto& c: ast.children) {
        switch (c.type) {
            case Ast::Unknown: break;
            // case Ast::StatList: eval_stat_list(c, vm); break;
            case Ast::VarDeclStat: eval_var_decl(c, vm); break;
            case Ast::AssignmentStat: eval_assignment(c, vm); break;
            case Ast::PrintStat: eval_print_stat(c, vm); break;
            case Ast::IfStat: eval_if_stat(c, vm); break;
            // case Ast::FuncLiteral: eval_func_literal(c, vm); break;
        }
    }
    vm.pop_scope();
}

void eval_module(Ast& ast, VM& vm) {
    for (auto& c: ast.children) {
        switch (c.type) {
            case Ast::FuncDef: eval_func_def(c, vm); break;
            case Ast::StructDef: eval_struct_def(c, vm); break;
        }
    }
}


#endif
