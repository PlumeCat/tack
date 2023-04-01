#pragma once

#include <list>
#include  <jlib/swiss_vector.h>
#include "program.h"
#include "value.h"

// emit an instruction with 3 8 bit operands
#define emit(op, r0, r1, r2)            output->instructions.emplace_back(Opcode::op, r0, r1, r2)

// emit an instruction with 1 u8 operand r and 1 u16 operand u
#define emit_u(op, r, u)                auto ins = Instruction{}; ins.opcode = Opcode::op; ins.r0 = r; ins.u1 = u; output->instructions.emplace_back(ins);

// rewrite an instruction with 3 8-bit operands
#define rewrite(pos, type, r0, r1, r2)  output->instructions[pos] = { Opcode::type, r0, r1, r2 };
#define label(name)                     auto name = output->instructions.size();
#define should_allocate(n)
#define handle(x)                       break; case AstType::x:


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
struct ScopeContext;
struct FunctionCompiler;

static const uint32_t MAX_REGISTERS = 256;
static const uint32_t STACK_FRAME_OVERHEAD = 4;

struct VariableContext {
    uint8_t reg;
    bool is_const = false;
};
struct ScopeContext {
    FunctionCompiler* compiler;
    ScopeContext* parent_scope = nullptr; // establishes a tree structure over FunctionContext::scopes
    bool is_top_level_scope = false;
    hash_map<string, VariableContext> bindings = hash_map<string, VariableContext>(1, 1); // variables

    // lookup a variable
    // if it lives in a parent function, code will be emitted to capture it
    VariableContext* lookup(const string& name, CompiledFunction* output);
};
struct FunctionCompiler {
    // AST node to compile
    // TODO: make it not a parameter
    const AstNode* node;
    // name of function (if any)
    string name;

    // compiler state
    array<RegisterState, MAX_REGISTERS> registers;
    list<ScopeContext> scopes;
    vector<CaptureInfo> captures;

    // lookup a variable in the current scope stack
    VariableContext* lookup(const string& name, CompiledFunction* output) { return scopes.back().lookup(name, output); }

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
    VariableContext* bind_register(const string& binding, uint8_t reg, bool is_const = false) {
        registers[reg] = BOUND;
        return &scopes.back().bindings.insert(binding, {reg, is_const})->second;
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

    void push_scope(ScopeContext* parent_scope, bool is_top_level = false) {
        scopes.emplace_back(ScopeContext { this, parent_scope, is_top_level});
    }
    void pop_scope() {
        // unbind all bound registers
        for (auto& b: scopes.back().bindings) {
            //registers[b.second.reg] = FREE; // TODO: un-disable once capture is working, and see if still works
        }
        scopes.pop_back();
    }   

    void compile_func(const AstNode& node, CompiledFunction* output, ScopeContext* parent_scope = nullptr) {
        // compile function literal
        // TODO: emit bindings for the N arguments
        push_scope(parent_scope, true);
        auto nargs = node.children[0].children.size(); // ParamDef
        for (auto i = 0; i < nargs; i++) {
            auto& param = node.children[0].children[i]; // Identifier
            bind_register(param.data_s, i, false); // TODO: could choose to make arguments const?
        }
        auto reg = compile(node.children[1], output);
        emit(RET, 0, 0, 0);
        pop_scope();
    }
    uint8_t compile(const AstNode& node, CompiledFunction* output) {
        switch (node.type) {
            case AstType::Unknown: {} break;
            handle(StatList) { should_allocate(0);
                push_scope(&scopes.back());
                for (auto& c: node.children) {
                    compile(c, output);
                    free_all_registers();
                }
                pop_scope();
                return 0xff;
            }
            handle(IfStat) {
                auto has_else = node.children.size() == 3;
                auto cond_reg = compile(node.children[0], output); // evaluate the expressi, outputon
                label(condjump);
                emit(CONDJUMP, cond_reg, 0, 0); // PLACEHOLDER
                free_register(cond_reg); // can be reused
                compile(node.children[1], output); // compile the if bo, outputdy
                label(endif);
                
                if (has_else) {
                    emit(JUMPF, 0, 0, 0); // PLACEHODLER
                    label(startelse);
                    compile(node.children[2], output);
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
                auto cond_reg = compile(node.children[0], output);
                label(condjump);
                emit(CONDJUMP, cond_reg, 0, 0);
                free_register(cond_reg);
                compile(node.children[1], output);
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
                auto reg = compile(node.children[1], output);
                bind_register(node.children[0].data_s, reg);
                return reg;
            }
            handle(ConstDeclStat) { should_allocate(1);
                auto reg = compile(node.children[1], output);
                bind_register(node.children[0].data_s, reg, true);
                return reg;
            }
            handle(AssignStat) { should_allocate(0);
                auto source_reg = compile(node.children[1], output);
                auto& lhs = node.children[0];
                if (lhs.type == AstType::Identifier) {
                    if (auto var = lookup(node.children[0].data_s, output)) {
                        if (var->is_const) {
                            throw runtime_error("can't reassign const variable");
                        }
                        // TODO: try and elide this MOVE
                        // if the moved-from register is "busy" but not "bound", then the value is not used again
                        // only need MOVE if assigning directly from another variable
                        // in which case MOVE is more like COPY
                        emit(MOVE, var->reg, source_reg, 0);
                    } else {
                        throw runtime_error("can't find variable: "s + node.children[0].data_s);
                    }
                } else if (lhs.type == AstType::IndexExp) {
                    auto array_reg = compile(lhs.children[0], output);
                    auto index_reg = compile(lhs.children[1], output);
                    emit(STORE_ARRAY, source_reg, array_reg, index_reg);
                    free_register(index_reg);
                    free_register(array_reg);
                } else if (lhs.type == AstType::AccessExp) {
                    auto obj_reg = compile(lhs.children[0], output);
                    // save identifier as string and load it
                    auto key_reg = allocate_register();
                    auto index = output->store_string(lhs.children[1].data_s);
                    emit_u(LOAD_CONST, key_reg, index);                    
                    emit(STORE_OBJECT, source_reg, obj_reg, key_reg);
                    free_register(obj_reg);
                    free_register(key_reg);
                }
                free_register(source_reg);
                return source_reg; // unused
            }
            handle(Identifier) { should_allocate(0);
                if (auto v = lookup(node.data_s, output)) {
                    return v->reg;
                } else {
                    throw runtime_error("can't find variable: "s + node.data_s);
                }
                return 0xff;
            }
            handle(NumLiteral) { should_allocate(1);
                auto n = node.data_d;
                auto out = allocate_register();
                auto index = output->store_number(node.data_d);
                emit_u(LOAD_CONST, out, index);
                return out;
            }
            handle(StringLiteral) { should_allocate(1);
                auto out = allocate_register();
                auto index = output->store_string(node.data_s);
                emit_u(LOAD_CONST, out, index);
                return out;
            }
            handle(FuncLiteral) { should_allocate(1);
                // note this may create some captures and emit READ_CAPTURE
                auto index = output->store_function();
                auto func = (CompiledFunction*)value_to_pointer(output->storage[index]);
                func->name = (node.children.size() == 3) ? node.children[2].data_s : "(anonymous)";
                
                auto compiler = FunctionCompiler();
                compiler.compile_func(node, func, &scopes.back());
                
                // load an initial closure
                auto out = allocate_register();
                emit_u(ALLOC_FUNC, out, index);

                // add a const binding if it's a named function
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
                    arg_regs.emplace_back(compile(node.children[1].children[i], output));
                }
                auto func_reg = compile(node.children[0], output); // LHS evaluates to functi, outputon

                // copy arguments to top of stack in sequence
                auto end_reg = get_end_register();
                for (auto i = 0; i < nargs; i++) {
                    emit(MOVE, end_reg + i + STACK_FRAME_OVERHEAD, arg_regs[i], 0);
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
                auto in1 = compile(node.children[0], output);
                emit(PRINT, in1, 0, 0);
                free_register(in1);
                return 0xff;
            }
            handle(ReturnStat) {
                if (node.children.size()) {
                    auto return_register = compile(node.children[0], output);
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
                auto in = compile(node.children[0], output);
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
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(ADD, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(SubExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(SUB, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(LessExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(LESS, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(LessEqExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(LESSEQ, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(EqExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(EQUAL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(NotEqExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(NEQUAL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(GreaterExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(GREATER, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(GreaterEqExp) {
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(GREATEREQ, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(MulExp) { // 1 out
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
                auto out = allocate_register();
                emit(MUL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(DivExp) { // 1 out
                auto in1 = compile(node.children[0], output);
                auto in2 = compile(node.children[1], output);
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
                auto arr = compile(node.children[0], output);
                auto value = compile(node.children[1], output);
                emit(SHL, arr, value, 0);
                return 0xff;
            }
            handle(IndexExp) {
                auto arr = compile(node.children[0], output);
                auto ind = compile(node.children[1], output);
                auto out = allocate_register();
                emit(LOAD_ARRAY, out, arr, ind);
                free_register(arr);
                free_register(ind);
                return out;
            }
            handle(AccessExp) {
                auto obj = compile(node.children[0], output);
                
                // save identifier as string and load it
                auto key = allocate_register();
                auto index = output->store_string(node.children[1].data_s);
                emit_u(LOAD_CONST, key, index);
                
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

};

VariableContext* ScopeContext::lookup(const string& name, CompiledFunction* output) {
    if (auto iter = bindings.find(name); iter != bindings.end()) {
        return &iter->second;
    }

    if (parent_scope) {
        // not found by parent scope
        if (auto var = parent_scope->lookup(name, output)) {
            if (is_top_level_scope) { // handle capture scenario
                // create a mirroring local variable
                auto mirror_reg = compiler->allocate_register();
                auto mirror = compiler->bind_register(name, mirror_reg, var->is_const);

                // record necessary capture into mirror variable
                output->capture_info.emplace_back(CaptureInfo { .source_register = var->reg, .dest_register = mirror_reg });

                // emit code to read into mirror variable
                emit(READ_CAPTURE, mirror_reg, output->capture_info.size() - 1, 0);

                // return the MIRROR variable not the original!
                return mirror;
            } else {
                // normal scenario
                return var;
            }
        }
    }

    return nullptr;
}

#undef handle
#undef should_allocate
#undef label
#undef rewrite
#undef emit_u
#undef emit
