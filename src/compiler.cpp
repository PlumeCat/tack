#include "compiler.h"
#include "parsing.h"
#include "interpreter.h"

#include <sstream>

// emit an instruction with 3 8 bit operands
#define emit(op, r0, r1, r2)            output->instructions.emplace_back(Instruction { Opcode::op, r0, { r1, r2 } });

// emit an instruction with 1 u8 operand r and 1 u16 operand u
#define emit_u(op, r, u)                output->instructions.emplace_back(Instruction { Opcode::op, .r0 = r, .u1 = u });

// rewrite an instruction with 3 8-bit operands
#define rewrite(pos, type, r0, r1, r2)  output->instructions[pos] = Instruction { Opcode::type, r0, r1, r2 };

// rewrite an instruction with 1 u8 operand r and 1 u16 operand u
#define rewrite_u(pos, type, r, u)      output->instructions[pos] = Instruction { Opcode::type, .r0 = r, .u1 = u };

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
    storage.emplace_back(value_from_string(&strings.back()));
    return storage.size() - 1;
}
uint16_t CodeFragment::store_function() {
    functions.emplace_back();
    storage.emplace_back(value_from_pointer(&functions.back()));
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

    // s << "  strings:\n";
    // i = 0;
    // for (auto& _s : strings) {
    //     s << "    " << i << ": " << _s << '\n';
    //     i++;
    // }

    for (auto& f : functions) {
        s << f.str();
    }
    return s.str();
}

Compiler::VariableContext* Compiler::lookup(const std::string& name) {
    return scopes.back().lookup(name);
}

// get a free register
uint8_t Compiler::allocate_register() {
    // TODO: lifetime analysis for variables
    // make the register available as soon as the variable is not used any more
    for (auto i = 0; i < MAX_REGISTERS; i++) {
        if (registers[i] == RegisterState::FREE) {
            registers[i] = RegisterState::BUSY;
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
Compiler::VariableContext* Compiler::bind_name(const std::string& binding, uint8_t reg, bool is_const) {
    if (is_global) {
        auto var = scopes.back().bindings.insert(binding, { 0, is_const, true, interpreter->next_gid });
        interpreter->next_gid++; // TODO: clumsy, try and improve this
        return &(var->second);
    } else {
        registers[reg] = RegisterState::BOUND;
        return &scopes.back().bindings.insert(binding, { reg, is_const })->second;
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
    for (auto i = 0; i < MAX_REGISTERS; i++) {
        free_register(i);
    }
}

void Compiler::push_scope(ScopeContext* parent_scope, bool is_top_level) {
    scopes.emplace_back(ScopeContext { this, parent_scope, is_top_level });
}
void Compiler::pop_scope() {
    // unbind all bound registers
    for (auto& b : scopes.back().bindings) {
        //registers[b.second.reg] = FREE; // TODO: un-disable once capture is working, and see if still works
    }
    scopes.pop_back();
}


void Compiler::compile_func(const AstNode* node, CodeFragment* output, ScopeContext* parent_scope) {
    // compile function literal
    this->output = output;
    this->node = node;
    push_scope(parent_scope, true);
    auto nargs = node->children[0].children.size(); // ParamDef
    
    // emit bindings for the N arguments
    for (auto i = 0; i < nargs; i++) {
        auto& param = node->children[0].children[i]; // Identifier
        bind_name(param.data_s, i, false); // TODO: could choose to make arguments const?
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
            // for i in iterable { ... }
            // i, iterable, block
            if (is_global) {
                error("for loop not supported at global scope");
            }

            auto& ident = node->children[0].data_s;
            push_scope(&scopes.back());
            auto reg_v = allocate_register(); // loop variable
            auto reg_b = child(1); // iterable
            auto reg_i = allocate_register(); // index: must start at 0
            auto reg_b_state = registers[reg_b]; // remember old register state
            auto reg_i_state = registers[reg_i];
            registers[reg_b] = RegisterState::BOUND; // HACK:
            registers[reg_i] = RegisterState::BOUND; // HACK:
            bind_name(ident, reg_v, false);
            emit_u(LOAD_I, reg_i, 0);
            label(forloop);
            emit(FOR_ITER, reg_v, reg_b, reg_i);
            label(skip_loop);
            emit(JUMPF, 0, 0, 0);
            child(2); // block
            label(loop_bottom);
            emit(INCREMENT, reg_i, 0, 0);
            emit_u(JUMPB, 0, uint16_t((loop_bottom + 1) - forloop));
            rewrite_u(skip_loop, JUMPF, 0, uint16_t((loop_bottom + 2) - skip_loop));
            pop_scope();
            registers[reg_b] = reg_b_state;
            registers[reg_i] = reg_i_state;
            return 0xff;
        }
        handle(ForStat2) {
            // for k, v in obj { ... }
            error("2-ary for loop not supported at global scope");
            return 0xff;
        }
        handle(ForStatInt) {
            // for i in a, b { ... }
            if (is_global) {
                error("for loop not supported at global scope");
            }
            auto& ident = node->children[0].data_s;
            push_scope(&scopes.back()); // new scope - only contains the loop variable, block will get own scope
            auto reg_a = child(1); // start value
            auto reg_b = child(2); // end value
            auto reg_b_state = registers[reg_b];
            registers[reg_b] = RegisterState::BOUND; // HACK: the block might try and free this register
            bind_name(ident, reg_a, false); // TODO: find better approach for globals
            label(forloop);
            emit(FOR_INT, reg_a, reg_b, 0);
            label(skip_loop);
            emit(JUMPF, 0, 0, 0);
            child(3); // block
            label(loop_bottom);
            emit(INCREMENT, reg_a, 0, 0); // TODO: messy
            emit_u(JUMPB, 0, uint16_t((loop_bottom + 1) - forloop));
            rewrite_u(skip_loop, JUMPF, 0, uint16_t((loop_bottom + 2) - skip_loop));
            pop_scope();
            registers[reg_b] = reg_b_state; // HACK
            return 0xff;
        }
        handle(VarDeclStat) {
            should_allocate(1);
            auto reg = child(1);
            auto var = bind_name(node->children[0].data_s, reg);
            if (is_global) {
                emit_u(WRITE_GLOBAL, reg, var->g_id);
            }
            return 0xff;
        }
        handle(ConstDeclStat) {
            should_allocate(1);
            auto reg = child(1);
            auto var = bind_name(node->children[0].data_s, reg, true);
            if (is_global) {
                emit_u(WRITE_GLOBAL, reg, var->g_id);
            }
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
                        // TODO: try and elide this MOVE
                        // if the moved-from register is "busy" but not "bound", then the value is not used again
                        // only need MOVE if assigning directly from another variable
                        // in which case MOVE is more like COPY
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
            error("can't find variable: " + node->data_s);
            return 0xff;
        }
        handle(NumLiteral) {
            should_allocate(1);
            auto n = node->data_d;
            auto out = allocate_register();
            auto index = output->store_number(node->data_d);
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
            // note this may create some captures and emit READ_CAPTURE
            auto index = output->store_function();
            auto func = (CodeFragment*)value_to_pointer(output->storage[index]);
            func->name = output->name + "::" + ((node->children.size() == 3) ? node->children[2].data_s : "(anonymous)");

            // add a const binding if it's a named function
            auto out = allocate_register();
            auto var = (VariableContext*)nullptr;
            if (node->children.size() == 3) {
                var = bind_name(node->children[2].data_s, out, true);
            }
            
            // compiler - must happen before ALLOC_FUNC because
            // child compiler can emit READ_CAPTURE into current scope
            auto compiler = Compiler { .interpreter = interpreter };
            compiler.compile_func(node, func, &scopes.back());

            // allocate new closure into new register
            emit_u(ALLOC_FUNC, out, index);
            if (var && var->is_global) {
                emit_u(WRITE_GLOBAL, out, var->g_id);
            }
            

            return out;
        }
        handle(CallExp) {
            should_allocate(1);
            auto nargs = (uint8_t)node->children[1].children.size();

            // compile the arguments first and remember which registers they are in
            auto arg_regs = std::vector<uint8_t> {};
            for (auto i = 0; i < nargs; i++) {
                arg_regs.emplace_back(compile(&node->children[1].children[i]));
            }
            auto func_reg = child(0); // LHS evaluates to function

            // copy arguments to top of stack in sequence
            auto end_reg = get_end_register();
            for (auto i = 0; i < nargs; i++) {
                emit(MOVE, uint8_t(end_reg + i + STACK_FRAME_OVERHEAD), arg_regs[i], 0);
            }

            emit(CALL, func_reg, nargs, end_reg);

            // return value goes in end-reg so mark it as used
            registers[end_reg] = RegisterState::BUSY;
            return end_reg; // return value copied to end register
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
        handle(LenExp) {
            auto in = child(0);
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

            // TODO: inefficient stack usage, try and elide some of these MOVEs, also with function calls
            // TODO: allow more than 256 elements?
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

            // TODO: key loading / stack usage a bit inefficient here as well
            // TODO: allow more than 256 elements?
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