#pragma once

#include <list>
#include  <jlib/swiss_vector.h>
#include "program.h"
#include "value.h"


bool is_integer(double d) {
    return trunc(d) == d;
}

bool is_small_integer(double d) {
    return is_integer(d) && 
        d <= std::numeric_limits<int16_t>::max() &&
        d >= std::numeric_limits<int16_t>::min();
}

enum RegisterState {
    FREE = 0,
    BUSY = 1,
    BOUND = 2
};

using StorageIndex = uint16_t;
struct FunctionContext;
struct ScopeContext;
struct Compiler2 {
    list<FunctionContext> funcs; // need to grow without invalidating pointers
    
    // returns the storage index of the start address
    FunctionContext& add_function(Program& program, ScopeContext* parent_scope, const AstNode* node);
    
    void compile(const AstNode& module, Program& program);
};

static const uint32_t MAX_REGISTERS = 256;
struct VariableContext {
    uint8_t reg;

    bool is_const = false;
    bool is_reassigned = false;
    bool is_captured = false;
    
    uint32_t first_read = 0;
    uint32_t last_read = 0;
    uint32_t first_write = 0;
    uint32_t last_write = 0;
};
struct CaptureContext {
    uint8_t reg;
    uint32_t frame_removal; // how many parent frames
};
struct ScopeContext {
    FunctionContext* function;
    ScopeContext* parent_scope = nullptr; // a reverse linked-list backed by the scopes vector
    bool is_top_level_scope = false;
    hash_map<string, VariableContext> bindings = hash_map<string, VariableContext>(1, 1); // variables
};
struct FunctionContext {
    Compiler2* compiler;
    const AstNode* node;
    ScopeContext* parent_scope = nullptr; // enclosing scope - might belong to parent function
    StorageIndex storage_index;
    string name;

    list<ScopeContext> scopes;
    ScopeContext* current_scope;
    hash_map<string, CaptureContext> captures;
    array<RegisterState, MAX_REGISTERS> registers;

    VariableContext& lookup(const string& name) {
        // VariableContext& ScopeContext::lookup(const string& name) {
        log("Lookup:", name);

        for (auto s = current_scope; s->parent_scope && !s->is_top_level_scope; s = s->parent_scope) {
            if (auto iter = s->bindings.find(name); iter != s->bindings.end()) {
                return iter->second;
            }
        }

        // for (auto& [k, v]: bindings) {
        //     log("     - ", k, (int)v.reg);
        // }
        // if (auto iter = bindings.find(name); iter != bindings.end()) {
        //     log("Found binding:", iter->first);
        //     return iter->second;
        // }

        // // if (is_top_level_scope) {
        //     // top level scope should examine captures before checking parent scope
        //     // if (auto iter = function->captures.find(name); iter != function->captures.end(); iter++) {
        //     //     log("Found capture: ", iter->first);
        //     //     return iter->second;
        //     // }
        // // }

        // // if (parent_scope) {
        // //     log("Trying parent scope");
        // //     auto& res = parent_scope->lookup(name);
        // //     if (is_top_level_scope) {
        // //         log("Captured", name, "from parent scope");
        // //         res.is_captured = true;
        // //         // function->captures[name] = &res;
        // //         auto& capture = function->captures.insert(name, {})->second;
        // //         capture.is_captured = true;
        // //         capture.reg = function->captures.size() - 1;
        // //         // safe to take pointer because parent scope will always have completely compiled
        // //         return capture;
        // //     }
        // //     return res;
        // // }

        // throw runtime_error("can't find variable: " + name);
    }

    // get a free register
    uint8_t allocate_register() {
        // TODO: lifetime analysis for variables
        // make the register available as soon as the variable is not used any more
        for (auto i = 0; i < MAX_REGISTERS; i++) {
            if (registers[i] == FREE) {
                registers[i] = BUSY;
                return i;
            }
        }
        throw runtime_error("Ran out of registers!");
    }

    // get the register immediately after the highest non-free register
    uint8_t get_end_register() {
        for (auto i = 255; i > 0; i--) {
            if (registers[i-1] != FREE) {
                return i;
            }
        }
        return 0;
    }

    // set a register as bound by a variable
    void bind_register(const string& binding, uint8_t reg, bool is_const = false) {
        registers[reg] = BOUND;
        current_scope->bindings.insert(binding, { reg, is_const });
    }
    
    // free a register if it's not bound
    void free_register(uint8_t reg) {
        if (registers[reg] != BOUND) {
            registers[reg] = FREE;
        }
    }

    // free all unbound registers
    void free_all_registers() {
        for (auto i = 0; i < MAX_REGISTERS; i++) {
            free_register(i);
        }
    }

    void push_scope(bool is_top_level = false) {
        scopes.emplace_back(ScopeContext { this, current_scope, is_top_level });
        current_scope = &scopes.back();
    }
    void pop_scope() {
        // unbind all bound registers
        for (auto& b: current_scope->bindings) {
            registers[b.second.reg] = FREE;
        }
        // scopes.pop_back();
        current_scope = current_scope->parent_scope;
    }
    

    
    // emit an instruction with 3 8 bit operands
    #define emit(op, r0, r1, r2)          program.instructions.emplace_back(Opcode::op, r0, r1, r2)

    // emit an instruction with 1 8-bit operand (r) and 1 16-bit operand (u)
    #define emit_u(op, r, u)                auto ins = Instruction{}; ins.opcode = Opcode::op; ins.r0 = r; ins.u1 = u; program.instructions.emplace_back(ins);

    // rewrite an instruction with 3 8-bit operands
    #define rewrite(pos, type, r0, r1, r2)  program.instructions[pos] = { Opcode::type, r0, r1, r2 };
    #define label(name)                     auto name = program.instructions.size();
    #define should_allocate(n)
    #define handle(x)                       break; case AstType::x:

    uint8_t compile(Program& program) {
        // compile function literal
        // TODO: emit bindings for the N arguments
        current_scope = parent_scope;
        push_scope(true);
        auto nargs = node->children[0].children.size(); // ParamDef
        for (auto i = 0; i < nargs; i++) {
            auto& param = node->children[0].children[i]; // Identifier
            bind_register(param.data_s, i, false); // TODO: could choose to make arguments const?
        }
        if (node->children.size() == 3) {
            name = node->children[2].data_s;
        } else {
            name = "(anonymous)";
        }
        auto reg = compile(node->children[1], program);
        emit(RET, 0, 0, 0);
        pop_scope();
    }
    uint8_t compile(const AstNode& node, Program& program, uint8_t target_register = 0xff) {
        switch (node.type) {
            case AstType::Unknown: {} break;

                // handle(StatList)
        
                // handle(ConstDeclStat)
                // handle(VarDeclStat)
                // handle(AssignStat)
                // handle(PrintStat)
            
                // handle(IfStat)
                // handle(WhileStat)
                // handle(ReturnStat)
            
                // handle(EqExp)
                // handle(NotEqExp)
                // handle(LessExp)
                // handle(GreaterExp)
                // handle(LessEqExp)
                // handle(GreaterEqExp)
            // handle(OrExp)
            // handle(AndExp)
            // handle(BitOrExp)
            // handle(BitAndExp)
            // handle(BitXorExp)
                // handle(ShiftLeftExp)
            // handle(ShiftRightExp)
                // handle(AddExp)
                // handle(SubExp)
                // handle(MulExp)
                // handle(DivExp)
            // handle(ModExp)
            // handle(PowExp)
            
            // handle(TernaryExp)
            // handle(NegateExp)
            // handle(NotExp)
            // handle(BitNotExp)
                // handle(LenExp)
            
                // handle(CallExp)
                // handle(ArgList)
                // handle(IndexExp)
                // handle(AccessExp)
            
                // handle(ClockExp)
                // handle(RandomExp)
            
                // handle(FuncLiteral)
                // handle(ParamDef)
                // handle(NumLiteral)
                // handle(StringLiteral)
                // handle(ArrayLiteral)
                // handle(ObjectLiteral)
                // handle(Identifier)

            handle(StatList) { should_allocate(0);
                push_scope();
                for (auto& c: node.children) {
                    compile(c, program);
                    free_all_registers();
                }
                pop_scope();
                return 0xff;
            }
            handle(IfStat) {
                auto has_else = node.children.size() == 3;
                auto cond_reg = compile(node.children[0], program); // evaluate the expression
                label(condjump);
                emit(CONDJUMP, cond_reg, 0, 0); // PLACEHOLDER
                free_register(cond_reg); // can be reused
                compile(node.children[1], program); // compile the if body
                label(endif);
                
                if (has_else) {
                    emit(JUMPF, 0, 0, 0); // PLACEHODLER
                    label(startelse);
                    compile(node.children[2], program);
                    label(endelse);
                    if (endelse - endif > 0xff) {
                        throw runtime_error("jump too much");
                    }
                    rewrite(endif, JUMPF, uint8_t(endelse - endif), 0, 0);
                    if (startelse - condjump > 0xff) {
                        throw runtime_error("jump too much");
                    }
                    rewrite(condjump, CONDJUMP, cond_reg, uint8_t(startelse - condjump), 0);
                } else {
                    if (endif - condjump > 0xff) {
                        throw runtime_error("jump too much");
                    }
                    rewrite(condjump, CONDJUMP, cond_reg, uint8_t(endif - condjump), 0);
                }
                return 0xff;
            }
            handle(WhileStat) {
                label(condeval);
                auto cond_reg = compile(node.children[0], program);
                label(condjump);
                emit(CONDJUMP, cond_reg, 0, 0);
                free_register(cond_reg);
                compile(node.children[1], program);
                label(jumpback);
                if (jumpback - condeval > 0xff) {
                    throw runtime_error("jump too much");
                }
                emit(JUMPB, uint8_t(jumpback - condeval), 0, 0);
                label(endwhile);
                if (endwhile - condjump > 0xff) {
                    throw runtime_error("jump too much");
                }
                rewrite(condjump, CONDJUMP, cond_reg, uint8_t(endwhile - condjump), 0);
                return 0xff;
            }
            handle(VarDeclStat) { should_allocate(1);
                auto reg = compile(node.children[1], program);
                bind_register(node.children[0].data_s, reg);
                return reg;
            }
            handle(ConstDeclStat) { should_allocate(1);
                auto reg = compile(node.children[1], program);
                bind_register(node.children[0].data_s, reg, true);
                return reg;
            }
            handle(AssignStat) { should_allocate(0);
                auto reg = compile(node.children[1], program);
                auto& lhs = node.children[0];
                if (lhs.type == AstType::Identifier) {
                    // TODO: try and elide this MOVE with 'target' register
                    auto& var = lookup(node.children[0].data_s);
                    if (var.is_const) {
                        throw runtime_error("can't reassign const");
                    }
                    var.is_reassigned = true;
                    emit(MOVE, var.reg, reg, 0);
                } else if (lhs.type == AstType::IndexExp) {
                    auto array_reg = compile(lhs.children[0], program);
                    auto index_reg = compile(lhs.children[1], program);
                    emit(STORE_ARRAY, reg, array_reg, index_reg);
                    free_register(index_reg);
                    free_register(array_reg);
                } else if (lhs.type == AstType::AccessExp) {
                    auto obj_reg = compile(lhs.children[0], program);

                    // save identifier as string and load it
                    auto key_reg = allocate_register();
                    auto in1 = program.storage.size();
                    program.strings.emplace_back(lhs.children[1].data_s);
                    program.storage.emplace_back(value_from_string(
                        &program.strings.back()
                    ));
                    emit_u(LOAD_CONST, key_reg, in1);
                    
                    emit(STORE_OBJECT, reg, obj_reg, key_reg);
                    free_register(obj_reg);
                    free_register(key_reg);
                }
                free_register(reg);
                return reg;
            }
            handle(Identifier) { should_allocate(0);
                // return bindings[node.data_s];
                return lookup(node.data_s).reg;
            }
            handle(NumLiteral) { should_allocate(1);
                auto n = node.data_d;
                auto out = allocate_register();
                auto in1 = program.storage.size();
                program.storage.emplace_back(value_from_number(n));
                emit_u(LOAD_CONST, out, in1);                
                return out;
            }
            handle(StringLiteral) { should_allocate(1);
                auto out = allocate_register();
                auto in1 = (uint8_t)program.storage.size();
                program.strings.emplace_back(node.data_s);
                program.storage.emplace_back(value_from_string(
                    &program.strings.back()
                ));
                emit(LOAD_CONST, out, in1, 0);
                return out;
            }
            handle(FuncLiteral) { should_allocate(1);
                auto& func = compiler->add_function(program, current_scope, &node);
                auto out = allocate_register();

                emit_u(ALLOC_FUNC, out, func.storage_index);
                for (auto& [ k, v ]: func.captures) {
                    // emit(CAPTURE, out, v.reg);
                    // need to grab V from the stack
                    // need absolute position for v
                }

                if (node.children.size() == 3) {
                    bind_register(node.children[2].data_s, out, true);
                }
                return out;
            }
            handle(CallExp) { should_allocate(1);
                auto nargs = node.children[1].children.size();

                // compile the arguments first and remember which registers they are in
                auto arg_regs = vector<uint8_t>{};
                for (auto i = 0; i < nargs; i++) {
                    arg_regs.emplace_back(compile(node.children[1].children[i], program));
                }
                auto func_reg = compile(node.children[0], program); // LHS evaluates to function

                // copy arguments to top of stack in sequence
                auto end_reg = get_end_register();
                for (auto i = 0; i < nargs; i++) {
                    emit(MOVE, end_reg + i + 2, arg_regs[i], 0);
                }
                
                emit(CALL, func_reg, nargs, end_reg);
                
                // return value goes in end-reg so mark it as used
                registers[end_reg] = BUSY;
                return end_reg; // return value copied to end register
            }
            handle(ClockExp) {
                auto reg = allocate_register();
                emit(CLOCK, reg, 0, 0);
                return reg;
            }
            handle(RandomExp) {
                auto reg = allocate_register();
                emit(RANDOM, reg, 0, 0);
                return reg;
            }
            handle(PrintStat) { // no output
                auto in1 = compile(node.children[0], program);
                emit(PRINT, in1, 0, 0);
                free_register(in1);
                return 0xff;
            }
            handle(ReturnStat) {
                if (node.children.size()) {
                    auto return_register = compile(node.children[0], program);
                    if (return_register == 0xff) {
                        throw runtime_error("return value incorrect register");
                        // emit(RET, 0, 0, 0);
                    }
                    emit(RET, 1, return_register, 0);
                } else {
                    emit(RET, 0, 0, 0);
                }
                return 0xff; // it's irrelevant
            }
            handle(LenExp) {
                auto in = compile(node.children[0], program);
                auto out = allocate_register();
                emit(LEN, out, in, 0);
                free_register(in);
                return out;
            }
            handle(AddExp) { // 1 out
                // TODO: experiment with the following:
                //      i1 = compile_node(...)
                //      free_register(i1)
                //      i2 = compile_node(...)
                //      free_register(i2)
                //      out = allocate_register()
                //      ...
                // allows to reuse one of the registers (if not bound)
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(ADD, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(SubExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(SUB, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(LessExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(LESS, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(LessEqExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(LESSEQ, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(EqExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(EQUAL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(NotEqExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(NEQUAL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(GreaterExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(GREATER, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(GreaterEqExp) {
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(GREATEREQ, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(MulExp) { // 1 out
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(MUL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(DivExp) { // 1 out
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(DIV, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(ArrayLiteral) {
                auto reg = allocate_register();
                // TODO: child elements
                emit(ALLOC_ARRAY, reg, 0, 0);
                return reg;
            }
            handle(ObjectLiteral) {
                auto reg = allocate_register();
                emit(ALLOC_OBJECT, reg, 0, 0);
                return reg;
            }
            handle(ShiftLeftExp) {
                auto arr = compile(node.children[0], program);
                auto value = compile(node.children[1], program);
                emit(SHL, arr, value, 0);
                return 0xff;
            }
            handle(IndexExp) {
                auto arr = compile(node.children[0], program);
                auto ind = compile(node.children[1], program);
                auto out = allocate_register();
                emit(LOAD_ARRAY, out, arr, ind);
                free_register(arr);
                free_register(ind);
                return out;
            }
            handle(AccessExp) {
                // TODO: object read
                auto obj = compile(node.children[0], program);
                
                // save identifier as string and load it
                auto key = allocate_register();
                auto in1 = (uint8_t)program.storage.size();
                program.strings.emplace_back(node.children[1].data_s);
                program.storage.emplace_back(value_from_string(
                    &program.strings.back()
                ));
                emit(LOAD_CONST, key, in1, 0);
                
                // load from object
                auto out = allocate_register();
                emit(LOAD_OBJECT, out, obj, key);
                free_register(obj);
                free_register(key);
                return out;
            }

            default: { throw runtime_error("unknown ast: " + to_string(node.type)); } break;
        }

        throw runtime_error("forgot to return a register");
        return 0;
    }

    #undef handle
    #undef should_allocate
    #undef label
    #undef rewrite
    #undef emit_u
    #undef emit
};

void Compiler2::compile(const AstNode& module, Program& program) {
    if (module.type != AstType::StatList) {
        throw runtime_error("expected statlist");
    }

    auto node = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef, {}), module);
    add_function(program, nullptr, &node);

    // compile all function literals and append to main bytecode
    // TODO: interesting, can this be parallelized?
    for (auto f = funcs.begin(); f != funcs.end(); f++) {
        // can't use range-based for because compiler.functions size changes
        auto start_addr = program.instructions.size();
        program.functions.emplace_back(FunctionType { (uint32_t)start_addr });
        program.storage[f->storage_index] = value_from_function(&program.functions.back());
        //value_from_integer(program.functions.size() - 1);
        
        f->compile(program);
    }

    // for each function, box captured variables
}
FunctionContext& Compiler2::add_function(Program& program, ScopeContext* parent_scope, const AstNode* node) {
    auto storage_index = program.storage.size();
    program.storage.emplace_back(value_null());
    funcs.emplace_back(FunctionContext{ this, node, parent_scope, (StorageIndex)storage_index });
    return funcs.back();
}


struct Interpreter2 {
    // Lua only allows 256 registers, we should do the same

    void execute(const Program& program) {
        #define handle(opcode) break; case Opcode::opcode:
        #define REGISTER(n) _stack[_stackbase+n]

        auto _stack = array<Value, 4096> {};
        _stack.fill(value_null());
        _stack[0]._i = program.instructions.size(); // pseudo return address
        _stack[1]._i = 0; // reset stack base to 0
        auto _stackbase = 2u;
        auto _pc = 0u;
        auto _pe = program.instructions.size();
        auto _arrays = list<ArrayType>{};
        auto _objects = list<ObjectType>{};
        auto _closures = list<FunctionType>{};
        auto _boxes = list<Value>{};
        auto error = [&](auto err) { throw runtime_error(err); return value_null(); };

        auto dumpstack = [&] {
            log<true, false>(" ---- Stack ---- ");
            for (auto i = 0; i < 20 && value_get_type(REGISTER(i)) != Type::Null; i++) {
                log<true, false>(i, ": ", REGISTER(i));
            }
            log<true, false>(" --------------- ");

        };

        try {
            while (_pc < _pe) {
                auto i = program.instructions[_pc];
                switch (i.opcode) {
                    case Opcode::UNKNOWN: break;
                    handle(LOAD_I) {
                        REGISTER(i.r0) = value_from_integer(i.s1);
                    }
                    handle(LOAD_CONST) {
                        REGISTER(i.r0) = program.storage[i.r1];
                    }
                    handle(ADD) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) + value_to_number(REGISTER(i.r2)));
                    }
                    handle(SUB) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) - value_to_number(REGISTER(i.r2)));
                    }
                    handle(MUL) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) * value_to_number(REGISTER(i.r2)));
                    }
                    handle(DIV) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) / value_to_number(REGISTER(i.r2)));
                    }
                    // TODO: Bin ops need type checking
                    handle(SHL) {
                        auto* arr = value_to_array(REGISTER(i.r0));
                        arr->emplace_back(REGISTER(i.r1));
                    }
                    handle(LESS) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) <
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(EQUAL) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) ==
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(NEQUAL) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) !=
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(LESSEQ) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) <=
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(GREATER) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) >
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(GREATEREQ) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) >=
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(MOVE) { REGISTER(i.r0) = REGISTER(i.r1); }
                    handle(CONDJUMP) {
                        auto val = REGISTER(i.r0);
                        if (!(
                            (value_get_type(val) == Type::Boolean && value_to_boolean(val)) ||
                            (value_get_type(val) == Type::Integer && value_to_integer(val)) ||
                            (value_get_type(val) == Type::Number && value_to_number(val))
                        )) { _pc += i.r1 - 1; }
                    }
                    handle(JUMPF) { _pc += i.r0 - 1; }
                    handle(JUMPB) { _pc -= i.r0 + 1; }
                    handle(LEN) {
                        auto* arr = value_to_array(REGISTER(i.r1));
                        REGISTER(i.r0) = value_from_number(arr->size());
                    }
                    handle(ALLOC_FUNC) {
                        REGISTER(i.r0) = program.storage[i.u1];
                        // auto func = value_to_function(program.storage[i.u1]);
                        // _closures.emplace_back(func);
                        // REGISTER(i.r0) = value_from_function(&_closures.back());
                    }
                    handle(ALLOC_ARRAY) {
                        _arrays.emplace_back(ArrayType{});
                        REGISTER(i.r0) = value_from_array(&_arrays.back());
                    }
                    handle(ALLOC_OBJECT) {
                        _objects.emplace_back(ObjectType{});
                        REGISTER(i.r0) = value_from_object(&_objects.back());
                    }
                    handle(LOAD_ARRAY) {
                        auto arr_val = REGISTER(i.r1);
                        auto ind_val = REGISTER(i.r2); 
                        auto* arr = value_to_array(arr_val);
                        auto ind_type = value_get_type(ind_val);
                        auto ind = (ind_type == Type::Integer)
                            ? value_to_integer(ind_val) 
                            : (ind_type == Type::Number) 
                                ? (int)value_to_number(ind_val)
                                : error("expected number")._i;

                        if (ind >= arr->size()) {
                            error("outof range index");
                        }
                        REGISTER(i.r0) = (*arr)[ind];
                    }
                    handle(STORE_ARRAY) {
                        auto arr_val = REGISTER(i.r1);
                        if (value_get_type(arr_val) != Type::Array) {
                            error("expected array");
                        }
                        auto* arr = value_to_array(arr_val);
                        
                        auto ind_val = REGISTER(i.r2); 
                        auto ind_type = value_get_type(ind_val);
                        auto ind = (ind_type == Type::Integer)
                            ? value_to_integer(ind_val) 
                            : (ind_type == Type::Number) 
                                ? (int)value_to_number(ind_val)
                                : error("expected number")._i;
                        
                        if (ind > arr->size()) {
                            error("outof range index");
                        }
                        (*arr)[ind] = REGISTER(i.r0);
                    }
                    handle(LOAD_OBJECT) {
                        auto obj_val = REGISTER(i.r1);
                        auto key_val = REGISTER(i.r2);
                        auto* obj = value_to_object(obj_val);
                        if (value_get_type(key_val) != Type::String) {
                            error("expected string key");
                        }
                        auto iter = obj->find(*value_to_string(key_val));
                        if (iter == obj->end()) {
                            error("key not found");
                        }
                        REGISTER(i.r0) = iter->second;
                    }
                    handle(STORE_OBJECT) {
                        auto obj_val = REGISTER(i.r1);
                        auto key_val = REGISTER(i.r2);
                        auto* obj = value_to_object(obj_val);
                        if (value_get_type(key_val) != Type::String) {
                            error("expected string key");
                        }
                        (*obj)[*value_to_string(key_val)] = REGISTER(i.r0);
                    }
                    handle(CALL) {
                        auto func_start = value_to_function(REGISTER(i.r0))->code;
                        auto nargs = i.r1;
                        auto new_base = i.r2;
                        REGISTER(new_base)._i = _pc; // push return addr
                        REGISTER(new_base+1)._i = _stackbase; // push return frameptr
                        _pc = func_start - 1; // jump to addr
                        _stackbase = _stackbase + new_base + 2; // new stack frame
                    }
                    handle(RANDOM) {
                        REGISTER(i.r0) = value_from_number(rand() % 10000);
                    }
                    handle(CLOCK) {
                        REGISTER(i.r0) = value_from_number(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count() / 1e6);
                    }
                    handle(RET) {
                        auto return_addr = REGISTER(-2);
                        auto return_fptr = REGISTER(-1);
                        REGISTER(-1) = value_null();
                        if (i.r0) {
                            REGISTER(-2) = REGISTER(i.r1);
                        } else {
                            REGISTER(-2) = value_null();
                        }
                        _pc = return_addr._i;
                        _stackbase = return_fptr._i;
                    }
                    handle(PRINT) {
                        log<true,false>(REGISTER(i.r0));
                    }
                    break; default: error("unknown instruction: " + to_string(i.opcode));
                }
                _pc++;
            }
        }
        catch (exception& e) {
            // stack unwind
            log("runtime error: "s, e.what());
            auto fp = _stackbase;
            while (true) {
                auto return_addr = _stack[fp - 2]._i;
                fp = value_to_integer(_stack[fp - 1]);
                log("return addr: ", return_addr, fp);
                if (return_addr == program.instructions.size()) {
                    break;
                }
            }
        }
    }
};