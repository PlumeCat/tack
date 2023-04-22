#include "compiler.h"
#include "parsing.h"
#include "interpreter.h"

#include <sstream>

// emit an instruction with 3 8 bit operands
#define emit(op, r0, r1, r2)            output->instructions.emplace_back(Instruction { Opcode::op, r0, { r1, r2 } });

// emit an instruction with 1 u8 operand r and 1 u16 operand u
#define emit_u(op, r, u)                output->instructions.emplace_back(Instruction { .opcode = Opcode::op, .r0 = r, .u1 = u });

// rewrite an instruction with 3 8-bit operands
#define rewrite(pos, type, r0, r1, r2)  output->instructions[pos] = Instruction { Opcode::type, r0, r1, r2 };

// rewrite an instruction with 1 u8 operand r and 1 u16 operand u
#define rewrite_u(pos, type, r, u)      output->instructions[pos] = Instruction { .opcode = Opcode::type, .r0 = r, .u1 = u };

#define label(name)                     auto name = output->instructions.size();
#define should_allocate(n)
#define handle(x)                       break; case AstType::x:
#define error(msg)                      throw std::runtime_error("compiler error: " msg)
#define child(n)                        compile(&node->children[n]);



uint16_t CodeFragment::store_number(double d) {
    storage.emplace_back(value_from_number(d));
    return storage.size() - 1;
}

uint16_t CodeFragment::store_string(const std::string & data) {
    strings.emplace_back(data);
    storage.emplace_back(value_from_string(strings.back().c_str()));
    return storage.size() - 1;
}
uint16_t CodeFragment::store_fragment(CodeFragment* fragment) {
    storage.emplace_back(value_from_pointer(fragment));
    return storage.size() - 1;
}

std::string CodeFragment::str() {
    auto s = std::stringstream {};
    s << "function: " << name << std::endl;
    auto i = 0;
    for (auto bc : instructions) {
        s << "    " << i << ": " << ::to_string(bc.opcode) << ' ' << (uint32_t)bc.r0 << ' ' << (uint32_t)bc.r1 << ' ' << (uint32_t)bc.r2 << '\n';
        i++;
    }

    s << "  storage:\n";
    i = 0;
    for (auto& x : storage) {
        s << "    " << i << ": " << x << '\n';
        i++;
    }

    return s.str();
}

Compiler::VariableContext* Compiler::lookup(const std::string& name) {
    return scopes.back().lookup(name);
}

// get a free register
uint8_t Compiler::allocate_register() {
    for (auto i = 0u; i < MAX_REGISTERS; i++) {
        if (registers[i] == RegisterState::FREE) {
            registers[i] = RegisterState::BUSY;
            output->max_register = std::max(output->max_register, (uint32_t)i);
            return i;
        }
    }
    error("Ran out of registers!");
}

// get 2 registers next to each other
uint8_t Compiler::allocate_register2() {
    for (auto i = 0u; i < MAX_REGISTERS - 1; i++) {
        if (registers[i] == RegisterState::FREE &&
            registers[i+1] == RegisterState::FREE) {
            registers[i] = RegisterState::BUSY;
            registers[i+1] = RegisterState::BUSY;
            return i;
        }
    }
    error("Ran out of registers!");
}

// get the register immediately after the highest non-free register
uint8_t Compiler::get_end_register() {
    for (auto i = 255; i > 0; i--) {
        if (registers[i - 1] != RegisterState::FREE) {
            return i;
        }
    }
    return 0;
}

// set a register as bound by a variable
Compiler::VariableContext* Compiler::bind_name(const std::string& binding, uint8_t reg, bool is_const, bool is_export) {
    if (is_export) {
        return interpreter->set_global(binding, is_const, value_null());
    } else {
        registers[reg] = RegisterState::BOUND;
        return &scopes.back().bindings.try_emplace(binding, VariableContext { reg, is_const }).first->second;
    }
}

// free a register if it's not bound
void Compiler::free_register(uint8_t reg) {
    if (registers[reg] != RegisterState::BOUND) {
        registers[reg] = RegisterState::FREE;
    }
}

// free all unbound registers
void Compiler::free_all_registers() {
    for (auto i = 0u; i < MAX_REGISTERS; i++) {
        free_register(i);
    }
}

void Compiler::push_scope(ScopeContext* parent_scope, bool is_top_level) {
    scopes.emplace_back(ScopeContext { this, parent_scope, is_top_level });
}
void Compiler::pop_scope() {
    // unbind all bound registers
    // for (auto& b : scopes.back().bindings) {
        // registers[b.second.reg] = RegisterState::FREE;
    // }
    scopes.pop_back();
}


void Compiler::compile_func(const AstNode* node, CodeFragment* output, ScopeContext* parent_scope) {
    // compile function literal
    this->output = output;
    this->node = node;
    push_scope(parent_scope, true);
    auto nargs = node->children[0].children.size(); // ParamDef
    
    // emit bindings for the N arguments
    for (auto i = 0u; i < nargs; i++) {
        auto& param = node->children[0].children[i]; // Identifier
        bind_name(param.data_s, i, false);
        // NOTE: impossible to be global
    }
    child(1);
    emit(RET, 0, 0, 0);
    pop_scope();
}


Compiler::VariableContext* Compiler::ScopeContext::lookup(const std::string& name) {
    // lookup local
    if (auto iter = bindings.find(name); iter != bindings.end()) {
        return &iter->second;
    }

    // lookup capture in parent scope
    if (parent_scope) {
        if (auto var = parent_scope->lookup(name)) {
            if (var->is_global) {
                return var;
            }
            if (is_function_scope) { // handle capture scenario
                // create a mirroring local variable
                // NOTE: impossible to be global
                auto mirror_reg = compiler->allocate_register();
                auto mirror = compiler->bind_name(name, mirror_reg, var->is_const);

                // record necessary capture into mirror variable
                compiler->output->capture_info.emplace_back(CaptureInfo { .source_register = var->reg, .dest_register = mirror_reg });

                // emit code to read into mirror variable
                compiler->emit(READ_CAPTURE, mirror_reg, uint8_t(compiler->output->capture_info.size() - 1), 0);

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


uint8_t Compiler::compile(const AstNode* node) {
    switch (node->type) {
    case AstType::Unknown: {} break;
        // statements
        handle(StatList) {
            should_allocate(0);
            push_scope(&scopes.back());
            for (auto& c : node->children) {
                compile(&c);
                free_all_registers();
            }
            pop_scope();
            return 0xff;
        }
        handle(IfStat) {
            auto has_else = node->children.size() == 3;
            auto cond_reg = child(0); // evaluate the expressi, outputon
            emit(CONDSKIP, cond_reg, 0, 0);
            free_register(cond_reg); // can be reused
            
            label(skip_if);
            emit(JUMPF, 0, 0, 0); // jump over if body
            child(1); // if body
            label(endif);

            if (has_else) {
                label(skip_else);
                emit(JUMPF, 0, 0, 0); // jump over else body
                child(2); // else body
                label(endelse);

                rewrite_u(skip_if,   JUMPF, 0, uint16_t((skip_else+1) - skip_if));
                rewrite_u(skip_else, JUMPF, 0, uint16_t(endelse - skip_else));
            } else {
                rewrite_u(skip_if, JUMPF, 0, uint16_t(endif - skip_if));
            }
            return 0xff;
        }
        handle(WhileStat) {
            label(condeval);
            auto cond_reg = child(0);
            emit(CONDSKIP, cond_reg, 0, 0);
            free_register(cond_reg);
            label(skip_loop);
            emit(JUMPF, 0, 0, 0);
            child(1); // block
            label(jumpback);
            emit_u(JUMPB, 0, uint16_t(jumpback - condeval));
            rewrite_u(skip_loop, JUMPF, 0, uint16_t((jumpback + 1) - skip_loop));
            return 0xff;
        }
        handle(ForStat) {
            auto& ident = node->children[0].data_s;
            push_scope(&scopes.back());
            auto reg_var = allocate_register(); // loop variable
            auto reg_iter = child(1); // iterable
            auto reg_ptr = allocate_register(); // index / loop state (pointer for object, )
            auto reg_ptr_state = registers[reg_ptr]; // remember old register state
            auto reg_iter_state = registers[reg_iter];
            registers[reg_ptr] = RegisterState::BOUND; // HACK:
            registers[reg_iter] = RegisterState::BOUND; // HACK:
            bind_name(ident, reg_var, false);

            emit(FOR_ITER_INIT, reg_ptr, reg_iter, 0);
            label(forloop);
            emit(FOR_ITER, reg_ptr, reg_iter, reg_var);
            label(skip_loop);
            emit(JUMPF, 0, 0, 0);
            child(2); // block
            label(loop_bottom);
            emit(FOR_ITER_NEXT, reg_ptr, reg_iter, 0);
            emit_u(JUMPB, 0, uint16_t((loop_bottom + 1) - forloop));
            rewrite_u(skip_loop, JUMPF, 0, uint16_t((loop_bottom + 2) - skip_loop));
            pop_scope();
            registers[reg_iter] = reg_iter_state;
            registers[reg_ptr] = reg_ptr_state;
            return 0xff;
        }
        handle(ForStat2) {
            auto& i1 = node->children[0].data_s;
            auto& i2 = node->children[1].data_s;
            push_scope(&scopes.back());
            auto r1 = allocate_register2(); // loop variables
            auto r2 = r1 + 1;
            auto reg_iter = child(2);
            auto reg_ptr = allocate_register();
            auto reg_ptr_state = registers[reg_ptr];
            auto reg_iter_state = registers[reg_iter];
            registers[reg_ptr] = RegisterState::BOUND; // HACK:
            registers[reg_iter] = RegisterState::BOUND;
            bind_name(i1, r1, false);
            bind_name(i2, r2, false);

            emit(FOR_ITER_INIT, reg_ptr, reg_iter, 0);
            label(forloop);
            emit(FOR_ITER2, reg_ptr, reg_iter, r1);
            label(skip_loop);
            emit(JUMPF, 0, 0, 0);
            child(3);
            label(loop_bottom);
            emit(FOR_ITER_NEXT, reg_ptr, reg_iter, 0);
            emit_u(JUMPB, 0, uint16_t((loop_bottom + 1) - forloop));
            rewrite_u(skip_loop, JUMPF, 0, uint16_t((loop_bottom + 2) - skip_loop));
            pop_scope();
            registers[reg_iter] = reg_iter_state;
            registers[reg_ptr] = reg_ptr_state;
            return 0xff;
        }
        handle(ForStatInt) {
            auto& ident = node->children[0].data_s;
            push_scope(&scopes.back()); // new scope - only contains the loop variable, block will get own scope
            auto reg_a = child(1); // start value
            auto reg_b = child(2); // end value
            auto reg_b_state = registers[reg_b];
            registers[reg_b] = RegisterState::BOUND; // HACK: the block might try and free this register
            bind_name(ident, reg_a, false);
            label(forloop);
            emit(FOR_INT, reg_a, reg_b, 0);
            label(skip_loop);
            emit(JUMPF, 0, 0, 0);
            child(3); // block
            label(loop_bottom);
            emit(INCREMENT, reg_a, 0, 0);
            emit_u(JUMPB, 0, uint16_t((loop_bottom + 1) - forloop));
            rewrite_u(skip_loop, JUMPF, 0, uint16_t((loop_bottom + 2) - skip_loop));
            pop_scope();
            registers[reg_b] = reg_b_state; // HACK
            return 0xff;
        }
        handle(VarDeclStat) {
            should_allocate(1);
            auto reg = child(1);
            
            auto is_export = node->children[0].data_d;
            auto var = bind_name(node->children[0].data_s, reg, false, is_export);
            if (is_export) {
                emit_u(WRITE_GLOBAL, reg, var->g_id);
            }
            return 0xff;
        }
        handle(ConstDeclStat) {
            should_allocate(1);
            auto reg = child(1);

            auto is_export = node->children[0].data_d;
            auto var = bind_name(node->children[0].data_s, reg, true, is_export);
            if (is_export) {
                emit_u(WRITE_GLOBAL, reg, var->g_id);
            }
            return 0xff;
        }
        handle(FuncDeclStat) {
            // HACK: This is an amalgam of ConstDeclStat and FuncLiteral
            // Currently it has to be specially interleaved because recursive functions need to be
            // able to capture themselves by name, which is impossible with bind_name("recursive", compile(...))

            auto& ident = node->children[0].data_s;
            auto func = interpreter->create_fragment();
            func->name = output->name + "::" + ident;

            auto out = allocate_register();
            auto index = output->store_fragment(func);
            
            /* interleaved from ConstDeclStat */ auto is_export = node->children[0].data_d;
            /* interleaved from ConstDeclStat */ auto var = bind_name(ident, out, true, is_export);
            
            auto compiler = Compiler { .interpreter = interpreter };
            compiler.compile_func(&node->children[1], func, &scopes.back());
            emit_u(ALLOC_FUNC, out, index);
            
            /* interleaved from ConstDeclStat */ if (is_export) {
            /* interleaved from ConstDeclStat */    emit_u(WRITE_GLOBAL, out, var->g_id);
            /* interleaved from ConstDeclStat */ }

            return 0xff;
        }

        handle(AssignStat) {
            should_allocate(0);
            auto source_reg = child(1);
            auto& lhs = node->children[0];
            if (lhs.type == AstType::Identifier) {
                if (auto var = lookup(node->children[0].data_s)) {
                    if (var->is_const) {
                        error("can't reassign const variable");
                    } else {
                        if (var->is_global) {
                            emit_u(WRITE_GLOBAL, source_reg, var->g_id);
                        } else {
                            emit(MOVE, var->reg, source_reg, 0);
                        }
                    }
                } else {
                    error("can't find variable: " + node->children[0].data_s);
                }
            } else if (lhs.type == AstType::IndexExp) {
                auto array_reg = compile(&lhs.children[0]);
                auto index_reg = compile(&lhs.children[1]);
                emit(STORE_ARRAY, source_reg, array_reg, index_reg);
                free_register(index_reg);
                free_register(array_reg);
            } else if (lhs.type == AstType::AccessExp) {
                auto obj_reg = compile(&lhs.children[0]);
                // save identifier as string and load it
                auto key_reg = allocate_register();
                auto index = output->store_string(lhs.children[1].data_s);
                emit_u(LOAD_CONST, key_reg, index);
                emit(STORE_OBJECT, source_reg, obj_reg, key_reg);
                free_register(obj_reg);
                free_register(key_reg);
            }
            free_register(source_reg);
            return 0xff;
        }
        handle(ReturnStat) {
            if (node->children.size()) {
                auto return_register = child(0);
                if (return_register == 0xff) {
                    error("return value incorrect register");
                    // emit(RET, 0, 0, 0);
                }
                emit(RET, 1, return_register, 0);
            } else {
                emit(RET, 0, 0, 0);
            }
            return 0xff; // it's irrelevant
        }
        
        // expressions
        handle(Identifier) {
            should_allocate(0);
            if (auto v = lookup(node->data_s)) {
                if (v->is_global) {
                    auto reg = allocate_register();
                    emit_u(READ_GLOBAL, reg, v->g_id);
                    return reg;
                } else {
                    return v->reg;
                }
            }
            error("identifier: can't find variable: " + node->data_s);
            return 0xff;
        }
        handle(NumLiteral) {
            should_allocate(1);
            auto n = node->data_d;
            auto out = allocate_register();
            auto index = output->store_number(n);
            emit_u(LOAD_CONST, out, index);
            return out;
        }
        handle(StringLiteral) {
            should_allocate(1);
            auto out = allocate_register();
            auto index = output->store_string(node->data_s);
            emit_u(LOAD_CONST, out, index);
            return out;
        }
        handle(FuncLiteral) {
            should_allocate(1);
            auto func = interpreter->create_fragment();
            auto index = output->store_fragment(func);
            func->name = output->name + "::(anonymous)";

            auto out = allocate_register();            
            // compile must happen before ALLOC_FUNC because
            // child compiler can emit READ_CAPTURE into current scope
            auto compiler = Compiler { .interpreter = interpreter };
            compiler.compile_func(node, func, &scopes.back());

            // allocate new closure into new register
            emit_u(ALLOC_FUNC, out, index);

            return out;
        }
        handle(CallExp) {
            should_allocate(1);
            auto nargs = (uint8_t)node->children[1].children.size();

            // compile the arguments first and remember which registers they are in
            auto arg_regs = std::vector<uint8_t> {};
            for (auto i = 0u; i < nargs; i++) {
                arg_regs.emplace_back(compile(&node->children[1].children[i]));
            }
            auto func_reg = child(0); // LHS evaluates to function

            // copy arguments to top of stack in sequence
            auto return_reg = get_end_register();
            for (auto i = 0u; i < nargs; i++) {
                emit(MOVE, uint8_t(return_reg + i + STACK_FRAME_OVERHEAD), arg_regs[i], 0);
            }

            emit(CALL, func_reg, nargs, return_reg);

            // return value goes in end-reg so mark it as used
            registers[return_reg] = RegisterState::BUSY;
            return return_reg; // return value copied to end register
        }
        handle(LenExp) {
            auto in = child(0);
            auto out = allocate_register();
            emit(LEN, out, in, 0);
            free_register(in);
            return out;
        }
        handle(AddExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(ADD, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(SubExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(SUB, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(LessExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(LESS, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(LessEqExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(LESSEQ, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(EqExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(EQUAL, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(NotEqExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(NEQUAL, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(GreaterExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(GREATER, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(GreaterEqExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(GREATEREQ, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(MulExp) { // 1 out
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(MUL, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(DivExp) { // 1 out
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(DIV, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(ModExp) {
            auto in1 = child(0);
            auto in2 = child(1);
            auto out = allocate_register();
            emit(MOD, out, in1, in2);
            free_register(in1);
            free_register(in2);
            return out;
        }
        handle(ArrayLiteral) {
            auto array_reg = allocate_register();
            auto n_elems = (uint8_t)node->children.size();
            auto elem_regs = std::vector<uint8_t>{};
            for (auto n = 0; n < n_elems; n++) {
                auto r = child(n);
                elem_regs.emplace_back(r);
            }

            auto end_reg = get_end_register();
            for (auto n = 0; n < n_elems; n++) {
                emit(MOVE, uint8_t(end_reg + n), elem_regs[n], 0);
            }

            emit(ALLOC_ARRAY, array_reg, n_elems, end_reg);
            return array_reg;
        }
        handle(ObjectLiteral) {
            auto obj_reg = allocate_register();
            auto n_elems = (uint8_t)node->children.size();
            auto key_indices = std::vector<uint8_t>{};
            auto val_regs = std::vector<uint8_t>{};
            for (auto n = 0; n < n_elems; n++) {
                // each child node is an AssignStat
                auto key = output->store_string(node->children[n].children[0].data_s);
                auto val = compile(&node->children[n].children[1]);
                key_indices.emplace_back(key);
                val_regs.emplace_back(val);
            }

            auto end_reg = get_end_register();
            for (auto n = 0; n < n_elems; n++) {
                emit_u(LOAD_CONST, uint8_t(end_reg + n * 2), key_indices[n]);
                emit(MOVE, uint8_t(end_reg + n * 2 + 1), val_regs[n], 0);
            }

            emit(ALLOC_OBJECT, obj_reg, n_elems, end_reg);
            return obj_reg;
        }
        handle(ShiftLeftExp) {
            auto arr = child(0);
            auto value = child(1);
            emit(SHL, arr, value, 0);
            return 0xff;
        }
        handle(IndexExp) {
            auto arr = child(0);
            auto ind = child(1);
            auto out = allocate_register();
            emit(LOAD_ARRAY, out, arr, ind);
            free_register(arr);
            free_register(ind);
            return out;
        }
        handle(AccessExp) {
            auto obj = child(0);

            // save identifier as string and load it
            auto key = allocate_register();
            auto index = output->store_string(node->children[1].data_s);
            emit_u(LOAD_CONST, key, index);

            // load from object
            auto out = allocate_register();
            emit(LOAD_OBJECT, out, obj, key);
            free_register(obj);
            free_register(key);
            return out;
        }

    default: { error("unknown ast: " + to_string(node->type)); } break;
    }

    error("forgot to return a register");
    return 0;
}


#undef error
#undef handle
#undef should_allocate
#undef label
#undef rewrite
#undef emit_u
#undef emit
#undef child