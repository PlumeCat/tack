#pragma once

#include <cmath>


// tree walking reference interpreter
struct RefInterpreter {
    //struct HeapValue {
    //	virtual ~HeapValue() {}
    //};
    //struct Object : HeapValue {
    //	unordered_map<string, double> values;
    //};
    //struct Array : HeapValue {
    //	vector<double> values;
    //};
    //struct Function : HeapValue {
    //	const AstNode* code; // the code
    //};
    //
    //// nan tagging for references
    //vector<HeapValue*> heap;
    //vector<Object*> stack; // one object per stack frame, they are stored on the heap
    /*

    mark and sweep gc:
        gc_roots = everything that can be reached from current call stack
            local variables in stack frames for call stack
            global variables (~= local variables in stack frame 0)
            any frames closed over by frames in current call stack
            ... needs thought

        // mark
        VISITED = [ false for x in heap ]
        def visit(node) {
            VISITED[node] = true

            if node is array
                for x in node
                    visit(x)
        }

        // sweep
        for x in heap
            if not VISITED(x)
                deallocate(x)

    */

    vector<unordered_map<string, double>> scopes;
    vector<AstNode> functions; // interned functions
    vector<string> strings; // interned string literals

    RefInterpreter() {
        scopes.emplace_back(); // global variables
    }

    template<typename T>
    void push_scope(const T& x) {
        scopes.emplace_back(x);
    }
    void pop_scope() {
        scopes.pop_back();
    }

    template<bool should_exist = true>
    double& lookup(const string& name) {
        auto& scope = scopes.back();
        auto var = scopes.back().find(name);
        if constexpr (should_exist) {
            if (var == scope.end()) {
                throw runtime_error("Variable not found: " + name);
            }
            return var->second;
        } else {
            if (var != scope.end()) {
                // throw runtime_error("Variable already existed: " + name);
            }
            return scope[name];
        }
    }

    double eval(const AstNode& ast) {
        switch (ast.type) {
            case AstType::StatList: {
                for (auto& c : ast.children) {
                    auto val = eval(c);
                    if (c.type == AstType::ReturnStat) {
                        return val;
                    }
                }
                return 0;
            }
            case AstType::VarDeclStat: {
                lookup<false>(ast.children[0].data_s) = eval(ast.children[1]);
                return 0;
            }
            case AstType::AssignStat: {
                // evaluate the lhs to locate the storage for the new value
                // this is a bit messy: lhs must be a AccessExp, IndexExp, or Identifier, anything else is an error
                // if lhs is an AccessExp then lhs is { lhs-lhs, identifier }; lhs-lhs must evaluate to an object
                // if lhs is an IndexExp then lhs is { lhs-lhs, exp }; lhs-lhs must evaluate to an array
                // if lhs is an identifier, it's either in scope or captured

                // you can't have the lhs be a function call or any other expression until we have proper value categories
                // which enables to return some kind of 'locator-value' (like lvalues) from functions (and maybe expressions if we allow them to be overridden)

                auto& lhs = ast.children[0];
                if (lhs.type == AstType::IndexExp) {
                    /*
                    locator = eval(lhs.children[0]);
                    if locator.type != Array {
                        error "Not an array"
                    index = eval(lhs.children[1])
                    rhs = eval(ast.children[1])
                    locator[index].set(rhs)
                    */
                }
                else if (lhs.type == AstType::AccessExp) {
                    /*
                    locator = eval(lhs.children[0]);
                    if locator.type != Object
                        error "Not an object"
                    rhs = eval(ast.children[1])
                    ident = lhs.children[1];
                    locator[ident.data_s].set(rhs)
                    */
                }
                else if (lhs.type == AstType::Identifier) {
                    lookup(ast.children[0].data_s) = eval(ast.children[1]);
                }
                else {
                    throw runtime_error("LHS of an assignment must be an identifier, access expression, or index expression");
                }

                return 0;
            }
            case AstType::PrintStat: {
                log<true, false>("(print)", eval(ast.children[0]));
                return 0;
            }
            case AstType::IfStat: {
                if (eval(ast.children[0])) {
                    eval(ast.children[1]);
                } else {
                    if (ast.children.size() > 2) {
                        eval(ast.children[2]);
                    }
                }
                return 0;
            }
            case AstType::WhileStat: {
                while (eval(ast.children[0])) {
                    eval(ast.children[1]);
                }
                return 0;
            }
            case AstType::ReturnStat: {
                if (ast.children.size()) {
                    return eval(ast.children[0]);
                }
                return 0.0;
            }

            case AstType::TernaryExp: return eval(ast.children[0]) ? eval(ast.children[1]) : eval(ast.children[2]);
#define EVAL_BIN(type, op)\
            case AstType::type: return eval(ast.children[0]) op eval(ast.children[1]);
#define EVAL_BIN_C(type, c, op)\
            case AstType::type: return (c)eval(ast.children[0]) op (c)eval(ast.children[1]);
#define EVAL_BIN_F(type, expr)\
            case AstType::type: return expr(eval(ast.children[0]), eval(ast.children[1]));
            EVAL_BIN(OrExp, ||)
            EVAL_BIN(AndExp, &&)
            EVAL_BIN_C(BitOrExp, uint64_t, |)
            EVAL_BIN_C(BitAndExp, uint64_t, &)
            EVAL_BIN_C(BitXorExp, uint64_t, ^)
            EVAL_BIN(EqExp, ==)
            EVAL_BIN(NotEqExp, !=)
            EVAL_BIN(LessExp, <)
            EVAL_BIN(GreaterExp, >)
            EVAL_BIN(LessEqExp, <=)
            EVAL_BIN(GreaterEqExp, >=)
            EVAL_BIN_C(ShiftLeftExp, uint64_t, <<)
            EVAL_BIN_C(ShiftRightExp, uint64_t, >>)
            EVAL_BIN(AddExp, +);
            EVAL_BIN(SubExp, -);
            EVAL_BIN(MulExp, *);
            EVAL_BIN(DivExp, / );
            EVAL_BIN_C(ModExp, uint64_t, %);
            EVAL_BIN_F(PowExp, pow);

            case AstType::NegateExp: return -eval(ast.children[0]);
            case AstType::NotExp: return !eval(ast.children[0]);
            case AstType::BitNotExp: return ~(uint64_t)eval(ast.children[0]);

            case AstType::CallExp: {
                auto func = eval(ast.children[0]);
                if (!isnan(func)) {
                    throw runtime_error("Tried to call a non-function");
                }
                auto& fn = functions[nan_unpack_int(func)];

                // evaluate arguments
                auto s = unordered_map<string, double> {};

                // must be same arguments
                auto& param_def = fn.children[0];
                auto& arg_list = ast.children[1];
                if (param_def.children.size() != ast.children.size()) {
                    throw runtime_error("Tried to call function with " + to_string(ast.children.size()) + "args, expected" + to_string(param_def.children.size()));
                }
                for (auto i = 0; i < param_def.children.size(); i++) {
                    s.insert({ param_def.children[i].data_s, eval(arg_list.children[i]) });
                }
                push_scope(s);
                
                // evaluate function
                auto retval = eval(fn.children[1]);
                pop_scope();
                return retval;
            };
            // case AstType::ArgList: { log("ArgList should be unreachable"); return 0; }
            case AstType::IndexExp: { return 0; }
            case AstType::AccessExp: { return 0; }

            case AstType::FuncLiteral: {
                functions.emplace_back(ast);
                return nan_pack_int(functions.size() - 1);
            };
            // case AstType::ParamDef: { log("ParamDef should be unreachable!");  return 0; }
            case AstType::StringLiteral: {
                strings.emplace_back(ast.data_s);
                return nan_pack_int(strings.size() - 1);
            }
            case AstType::NumLiteral: { return ast.data_d; }
            case AstType::Identifier: { return lookup(ast.data_s); }

            case AstType::Unknown:default: log("WARNING: unknown AST"); return 0;
        }
    }
};
