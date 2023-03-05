#pragma once

#include <list>
#include <string>
#include <vector>

#include <jlib/hash_map.h>

#include "instructions.h"

using namespace std;

struct Program {
    vector<Instruction> instructions;
    vector<Value> storage; // program constant storage goes at the bottom of the stack for now
    vector<string> strings; // string constants and literals storage - includes identifiers for objects
    hash_map<uint32_t, string> addr_to_function;

    string to_string() {
        auto s = stringstream {};

        s << "Program:\n";
        s << "   Bytecode:\n";
        auto i = 0;
        for (auto bc : instructions) {
            auto op = (Opcode)uint16_t(bc >> 16);
            auto r1 = uint16_t(bc & 0xffff);
            s << "        " << i << ": " << ::to_string(op) << ", " << r1 << '\n'; //", " << r2 << ", " << r3 << '\n';
            i++;
        }

        s << "    Storage:\n";
        i = 0;
        for (auto& x : storage) {
            s << "        " << i << ": " << x << '\n';
            i++;
        }

        return s.str();
    }
};

struct VariableContext {
    string name;
    uint32_t stackpos;
    bool is_captured = false; // if captured by a lower scope, allocate on heap
};
struct ScopeContext {
    hash_map<string, VariableContext> variables;
};
struct FunctionContext {
    const AstNode* func_node; // before compilation
    vector<ScopeContext> scopes;
    FunctionContext* parent_context; // so we can capture variables from lexical parent scopes
    uint32_t storage_location;
    uint32_t variable_count = 0;

    // number string object array function
    
    // variable -> stack index relative to current stack frame
    int lookup_variable(const string& identifier) {
        // TODO: search in higher scopes starting from parent scope
        // if the variable is found in a parent scope, mark as is_captured
        // captured variables must be auto boxed and garbage collected/reference counted
        // (we can also do escape analysis, eg if a reference to child function does not escape a parent function,
        // then it can refer to captured variables via the stack)
        for (auto s = scopes.rbegin(); s != scopes.rend(); s++) {
            if (auto iter = (*s).variables.find(identifier); iter != (*s).variables.end()) {
                // log(identifier, " -> ", iter->second.stackpos);
                return iter->second.stackpos;
            }
        }
        return -1;
    }
    
    // set stack position for new variable
    // don't bind if already exists in current scope
    int bind_variable(const string& identifier) {
        if (scopes.back().variables.find(identifier) != scopes.back().variables.end()) {
            return -1;
        }
        auto stackpos = variable_count++;
        scopes.back().variables[identifier] = VariableContext { identifier, stackpos, false };
        return stackpos;
    }

    void push_scope() {
        scopes.push_back({});
    }
    void pop_scope() {
        scopes.pop_back();
    }
};
struct Compiler {    
    list<FunctionContext> funcs; // list not vector - need to push while iterating and not invalidate reference. queue would be even better

    void add_function(const AstNode* node, uint32_t storage_location = UINT32_MAX, FunctionContext* parent_context = nullptr) {
        funcs.emplace_back(FunctionContext { node, {}, parent_context, storage_location });
    }
    
    void compile(const AstNode& module, Program& program) {
        // treat the top level of the program as a function body
        // RET will be emitted for it - don't need a special EXIT instruction
        // when the program is entered, the 0th stack frame has the last instruction+1
        // as the return address, so the interpreter loop can terminate
        // (see 'execute')
        auto node = AstNode(
            AstType::FuncLiteral,
            AstNode(AstType::ParamDef, {}),
            module // parsing ensures that module is always a stat-list
        );

        add_function(&node);

        // compile all function literals and append to main bytecode
        // TODO: interesting, can this be parallelized?
        for (auto f = funcs.begin(); f != funcs.end(); f++) { // can't use range-based for because compiler.functions size changes
            auto& func = *f;
            if (func.storage_location != UINT32_MAX) {
                auto start_address = program.instructions.size();
                program.storage[func.storage_location] = value_from_function(start_address);
                // program.addr_to_function[start_address] = program.strings[program.storage[func.storage_location +1].d];
            }
            compile_function(func, program);
        }
    }

    #define encode(op, ...) encode_instruction(Opcode::op, __VA_ARGS__)
    #define emit(op, ...) program.instructions.push_back(encode(op, __VA_ARGS__));
    #define rewrite(instr, op, ...) program.instructions[instr] = encode(op, __VA_ARGS__);
    #define handle(type)                break; case AstType::type:
    #define handle_binexp(type, opcode) handle(type) { child(0); child(1); emit(opcode, 0); }
    #define child(n) compile_node(context, node.children[n], program);
    #define label(name) auto name = program.instructions.size();
    
    void compile_function(FunctionContext& context, Program& program) {
        auto num_args = (int)context.func_node->children[0].children.size();
        
        // grow stack to make space for variables (space required will be known later)
        label(grow); emit(GROW, 0);
        
        // handle arguments; bind as variables
        context.push_scope();
        for(const auto& arg: context.func_node->children[0].children) {
            // bind arg as variable
            // it's at the top of the stack so it's mutable
            auto stackpos = context.bind_variable(arg.data_s);
            if (stackpos == -1) {
                throw runtime_error("Compile error: duplicate argument: " + arg.data_s);
            }
        }
        // compile body
        compile_node(context, context.func_node->children[1], program);
        context.pop_scope();
        
        // return
        emit(RET, 0);
        
        // space for variables
        auto num_variables = (int)context.variable_count;
        auto extra_stack = max(0, num_variables - num_args); // extra space for arguments not needed, CALL will handle, we just need to allow space for variable bindings
        rewrite(grow, GROW, extra_stack);
    }

    void compile_node(FunctionContext& context, const AstNode& node, Program& program) {
        switch (node.type) {
            case AstType::Unknown: return;
            
            handle(StatList) {
                context.push_scope();
                for (auto i = 0; i < node.children.size(); i++) {
                    child(i); // TODO: discard expression statements from stack (ideally they aren't permitted)
                }
                context.pop_scope();
            }
            handle(VarDeclStat) {
                // make sure not already declared
                // TODO: allow shadowing in lower scopes?
                auto stackpos = context.bind_variable(node.children[0].data_s);
                if (stackpos == -1) {
                    throw runtime_error("Compile error: variable already exists: " + node.children[0].data_s);
                }
                child(1); // push result of expression
                emit(WRITE_VAR, stackpos); // put the value at the top of the stack into the variable at stackpos
            }
            handle(AssignStat) {
                auto& lhs = node.children[0];
                auto& rhs = node.children[1];

                if (lhs.type == AstType::AccessExp) {
                    // // pass
                    // compile_node(context, lhs.children[0], program); // object
                    // compile_node(context, lhs.children[1], program); // key
                    // child(1); // value
                    // emit(STORE_OBJECT, 0);
                } else if (lhs.type == AstType::IndexExp) {
                    // put array, index, value on the stack
                    compile_node(context, lhs.children[0], program); // array
                    compile_node(context, lhs.children[1], program); // index
                    child(1); // value
                    emit(STORE_ARRAY, 0);
                } else if (lhs.type == AstType::Identifier) {
                    // make sure variable exists
                    auto stackpos = context.lookup_variable(node.children[0].data_s);
                    if (stackpos == -1) {
                        throw runtime_error("Compile error: variable not found: " + node.children[0].data_s);
                    }
                    child(1);
                    emit(WRITE_VAR, stackpos); // value at top of stack into variable at stackpos
                } else {
                    throw runtime_error("Compile error: invalid LHS for assignment statement");
                }
                
            }
            handle(PrintStat) {
                child(0);
                emit(PRINT, 0);
            }
            handle(IfStat) {
                child(0); // evaluate the expression
                
                label(condjump);emit(CONDJUMP, 0); // PLACEHOLDER
                                child(1); // compile the if body
                label(endif);   emit(JUMPF, 0); // PLACEHODLER TODO: not needed if there's no else-block

                label(startelse);if (node.children.size() == 3)
                                child(2); // compile the else body
                label(endelse);
                
                // TODO: make more efficient
                // can be more efficient for the case when there's no else (probably)
                rewrite(condjump, CONDJUMP, startelse - condjump);
                rewrite(endif, JUMPF, endelse - endif);
            }
            handle(WhileStat) {
                label(cond);    child(0); // evaluate expression
                
                label(condjump);emit(CONDJUMP, 0); // placeholder
                                child(1); // compile the body
                label(jumpback);emit(JUMPB, jumpback - cond); // jump to top of loop
                label(ewhile);

                rewrite(condjump, CONDJUMP, ewhile - condjump);
            }
            
            handle(TernaryExp) {}
            handle(OrExp) {}
            handle(AndExp) {}
            handle(BitOrExp) {}
            handle(BitAndExp) {}
            handle(BitXorExp) {}
            
            handle_binexp(ShiftLeftExp, SHL)
            handle(ShiftRightExp) {}
            
            handle_binexp(EqExp, EQUAL)
            handle_binexp(NotEqExp, NEQUAL)
            handle_binexp(LessExp, LESS)
            handle_binexp(GreaterExp, GREATER)
            handle_binexp(LessEqExp, LESSEQ)
            handle_binexp(GreaterEqExp, GREATEREQ)
            
            // handle_binexp(AddExp, ADD)
            handle(AddExp) {
                auto& c0 = node.children[0];
                auto& c1 = node.children[1];
                //if (c0.type == AstType::NumLiteral && trunc(c0.data_d) == c0.data_d && abs(c0.data_d) <= INT16_MAX) {
                //    if (c1.type == AstType::NumLiteral && trunc(c1.data_d) == c1.data_d && abs(c1.data_d) <= INT16_MAX) {
                //        // const fold
                //        program.storage.emplace_back(value_from_number(c0.data_d + c1.data_d));
                //        emit(LOAD_CONST, program.storage.size() - 1);
                //    } else {
                //        child(1);
                //        // ADDI left
                //        int16_t _i[2] = { 0, int16_t(c0.data_d) };
                //        auto i = uint32_t(Opcode::ADDI) << 16 | *reinterpret_cast<uint32_t*>(_i);
                //        program.instructions.emplace_back(i);
                //    }
                //} else 

                static auto A = true;
                if (A && c1.type == AstType::NumLiteral && trunc(c1.data_d) == c1.data_d && abs(c0.data_d) <= INT16_MAX) {
                    A = false;
                    child(0);
                    // ADDI right (commutative)
                    int16_t _i[2] = { int16_t(c1.data_d), 0 };
                    auto i = uint32_t(Opcode::ADDI) << 16 | *reinterpret_cast<uint32_t*>(_i);
                    program.instructions.emplace_back(i);

                } else 
                {
                    // full add
                child(0);
                child(1);
                emit(ADD, 0);
                }
            }
            
            handle_binexp(SubExp, SUB)
            handle_binexp(MulExp, MUL)
            handle_binexp(DivExp, DIV)
            handle_binexp(ModExp, MOD)
            handle_binexp(PowExp, POW)

            handle(NegateExp) { child(0); emit(NEGATE, 0); }
            handle(NotExp)    { child(0); emit(NOT, 0); }
            handle(BitNotExp) { child(0); emit(BITNOT, 0); }
            handle(LenExp)    { child(0); emit(LEN, 0); }

            handle(IndexExp) {
                child(0); // push the array
                child(1); // push the index
                emit(LOAD_ARRAY, 0);
            }
            handle(AccessExp) {
                child(0); // push the object
                child(1); // push the string key
                emit(LOAD_OBJECT, 0);
            }

            // TODO: for loops
            // TODO: break and continue in loops (?)
            handle(ReturnStat) {
                // TODO: handle return value
                auto nret = node.children.size(); // should be 0 or 1
                if (nret) {
                    child(0); // push return value. will never need to be boxed
                }
                emit(RET, nret);
            }
            handle(CallExp) {
                // TODO: optimize direct calls vs indirect calls with some kind of symbol table?
                // RHS of call expression is an ArgList; evaluate all arguments
                auto nargs = node.children[1].children.size();
                // emit(DUMP, 0);
                emit(PRECALL, 0);
                emit(GROW, nargs); // allocate space for arguments
                child(1);          // push all arguments
                child(0);          // LHS of the call exp evaluates to a function address
                emit(CALL, nargs); // call function at top of stack
            }
            handle(ArgList) {
                // top of stack contains args. 
                for (auto i = 0; i < node.children.size(); i++) {
                    child(i); // push child value to stack
                }
            }
            handle(FuncLiteral) {
                // put a placeholder value in storage at first. when the program is linked, the placeholder is
                // overwritten with the real start address
                auto storage_index = program.storage.size();
                program.storage.emplace_back(value_from_function(0)); // insptr (placeholder value)
                program.storage.emplace_back(value_from_integer(program.strings.size())); // index of string name
                auto& name = (context.func_node->children.size() == 3) ? context.func_node->children[2].data_s : "(anonymous)";
                program.strings.emplace_back(name); // string name if there is one
                // funcs.emplace_back(pair { &node, func_storage }); // compile function
                add_function(&node, storage_index, &context);
                // load the function start address
                emit(LOAD_CONST, storage_index);

                /*
                TODO: closure capture
                need to emit code to grab all the captured variables when the function literal is created
                these go in a separate allocation something like an invisible array, which is used
                as a sort of secondary stack when the function is reading/writing variables

                eg 
                LOAD_CONST storage_index
                */
                

            }
            handle(ParamDef) {}
            handle(NumLiteral) {
                // TODO: don't store the same constant more than once
                auto val = value_null();
                if (trunc(node.data_d) == node.data_d) {
                    val = value_from_integer((int32_t)node.data_d);
                } else {
                    val = value_from_number(node.data_d);
                }
                program.storage.emplace_back(val);
                emit(LOAD_CONST, program.storage.size() - 1);
            }
            handle(StringLiteral) {
                // TODO: don't store the same string more than once
                // especially important for frequently-used object fields
                program.strings.emplace_back(node.data_s);
                emit(LOAD_STRING, program.strings.size() - 1);
            }
            handle(ArrayLiteral) {
                // push the contents of the array to stack
                // ALLOC_ARRAY will copy them
                for (auto i = 0; i < node.children.size(); i++) {
                    child(i);
                }
                emit(ALLOC_ARRAY, node.children.size());
                // emit STORE_ARRAY for each element
                //for (auto i = 0; i < node.children.size(); i++) {
                //    // re-push the array... bit annoying
                //    emit(PUSH, 0);
                //    // push index. index is 
                //    program.storage.emplace_back(value_from_integer(i));
                //    emit(LOAD_CONST, program.storage.size() - 1);
                //    // push value
                //    child(i);
                //    // store
                //    emit(STORE_ARRAY, 0);
                //}
            }
            handle(ObjectLiteral) {
                emit(ALLOC_OBJECT, 0);
                // TODO: emit STORE_OBJECT for any provided fields
            }
            handle(ClockExp) { emit(CLOCK, 0); } // TODO: remove
            handle(RandomExp) { emit(RANDOM, 0); } // TODO: remove
            handle(Identifier) {
                // reading a variable (writing is not encountered by recursive compile)
                // so the variable exists and points to a stackpos
                auto stackpos = context.lookup_variable(node.data_s);
                if (stackpos == -1) {
                    throw runtime_error("Compile error: variable not found: " + node.data_s);
                }
                emit(READ_VAR, stackpos);
            }
        }

        // TODO: constant folding
        // TODO: jump folding
        // TODO: mov folding
    }
    
    #undef encode
    #undef emit
    #undef handle
    #undef child
    #undef label
};
