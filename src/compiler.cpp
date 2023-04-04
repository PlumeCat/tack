#include "compiler.h"
#include "parsing.h"

#include <jlib/log.h>
#include <sstream>



// emit an instruction with 3 8 bit operands
#define emit(op, r0, r1, r2)            output->instructions.emplace_back(Opcode::op, r0, r1, r2);

// emit an instruction with 1 u8 operand r and 1 u16 operand u
#define emit_u(op, r, u)                auto ins = Instruction{}; ins.opcode = Opcode::op; ins.r0 = r; ins.u1 = u; output->instructions.emplace_back(ins);

// rewrite an instruction with 3 8-bit operands
#define rewrite(pos, type, r0, r1, r2)  output->instructions[pos] = { Opcode::type, r0, r1, r2 };
#define emit_at(pos, type, r0, r1, r2)  output->instructions.insert(pos, { Opcode::type, r0, r1, r2 });
#define label(name)                     auto name = output->instructions.size();
#define handle(x)                       break; case AstType::x:
#define error(msg)                      throw std::runtime_error("compiler error: " msg)
#define child(n)                        compile(&node->children[n])
#define lookup_var(name)                scopes.back().lookup(name)


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
        s << "    " << i << " | " << ::to_string(bc.opcode) << ' ' << (uint32_t)bc.r0 << ' ' << (uint32_t)bc.r1 << ' ' << (uint32_t)bc.r2 << '\n';
        i++;
    }

    s << "  storage:\n";
    i = 0;
    for (auto& x : storage) {
        s << "    " << i << " | " << x << '\n';
        i++;
    }

    s << "  strings:\n";
    i = 0;
    for (auto& _s : strings) {
        s << "    " << i << " | " << _s << '\n';
        i++;
    }

    i = 0;
    s << "  captures:\n";
    for (auto& c: capture_info) {
        s << "    " << i << " | " << (int)c.source_register << " -> " << (int)c.dest_register << '\n';
        i++;
    }

    for (auto& f : functions) {
        s << f.str();
    }
    return s.str();
}


Compiler::VariableContext* Compiler::ScopeContext::lookup(const std::string& name) {
    if (auto iter = bindings.find(name); iter != bindings.end()) {
        return &iter->second;
    }

    if (parent_scope) {
        // not found by parent scope
        if (auto var = parent_scope->lookup(name)) {
            if (is_top_level_scope) {
                // we just captured something from an enclosing function
                // create a mirroring local variable
                auto mirror = compiler->bind_register(name, var->is_const);
                mirror->is_box = true;
                // emit code to read into mirror variable
                compiler->emit(READ_CAPTURE, mirror->reg, compiler->output->capture_info.size(), 0);
                // record capture into mirror variable
                compiler->output->capture_info.emplace_back(CaptureInfo { .source_register = var->reg, .dest_register = mirror->reg });

                if (!var->is_box) {
                    var->is_box = true;

                    // add an ALLOC_BOX in the parent scope
                    parent_scope->compiler->output->instructions[var->declaration] = {
                        Opcode::ALLOC_BOX, var->reg, 0, 0
                    };
                    // parent_scope->compiler->emit_at(
                    //     parent_scope->compiler->output->instructions.begin() + var->declaration,
                    //     ALLOC_BOX, var->reg, 0, 0
                    // );
                    
                    // rewrite all MOVE as WRITE_BOX or READ_BOX
                    for (auto i = var->declaration + 1; i < parent_scope->compiler->output->instructions.size(); i++) {
                        auto& ins = parent_scope->compiler->output->instructions[i];
                        if (ins.opcode == Opcode::MOVE) {
                            if (ins.r0 == var->reg) {
                                // WRITE_BOX
                                ins.opcode = Opcode::WRITE_BOX;
                            } else if (ins.r1 == var->reg) {
                                // READ_BOX
                                ins.opcode = Opcode::READ_BOX;
                            }
                        }
                    }

                }
                // it's safe to emit this into the parent compiler, any control flow
                // will not have finished compiling, so it will still emit correct jumps
                // parent_scope->compiler->emit(ALLOC_BOX, )
                
                // TODO: emit WRITE_BOX for subsequent writes (any MOVE with r0 == var->reg)
                // any MOVE gets replaced with WRITE_BOX 1:1 so no issue with control flow
                
                
                // TODO: emit READ_BOX for subsequent reads (any MOVE with r1 == var->reg)
                // problem for control flow eg:

                // let i = 0
                // if (x) {
                //      x = i
                // }
                // ...
                // ...
                // fn() { return i + 1 }

                // needs to emit READ_BOX for i in the loop, but can't change the jump :(


                // keep list of reads?
                // emit no-op MOVE for reads?

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
Compiler::VariableContext* Compiler::bind_register(const std::string& binding, bool is_const) {
    auto reg = allocate_register();
    registers[reg] = RegisterState::BOUND;
    label(decl);
    emit(NOOP, reg, 0, 0); // placeholder in case ALLOC_BOX is needed
    return &scopes.back().bindings.insert(binding, { (uint32_t)decl, reg, is_const })->second;
}

// free a register if it's not bound
void Compiler::free_register(uint8_t reg) {
    if (registers[reg] == RegisterState::BUSY) {
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
    push_scope(parent_scope, true);
    auto nargs = node->children[0].children.size(); // ParamDef
    for (auto i = 0; i < nargs; i++) {
        auto& param = node->children[0].children[i]; // Identifier
        auto var = bind_register(param.data_s, false); // TODO: could choose to make arguments const?
        // move register i into argument variable
        emit(MOVE, var->reg, i, 0);

    }
    auto reg = child(1);
    emit(RET, 0, 0, 0);
    pop_scope();
}


uint8_t Compiler::compile(const AstNode* node) {
    switch (node->type) {
    case AstType::Unknown: {} break;
        handle(StatList) {
            push_scope(&scopes.back());
            for (auto i = 0; i < node->children.size(); i++) {
                child(i);
                free_all_registers();
            }
            pop_scope();
            return 0xff;
        }
        handle(IfStat) {
            auto has_else = node->children.size() == 3;
            auto cond_reg = child(0);
            label(condjump);
            emit(CONDJUMP, cond_reg, 0, 0); // PLACEHOLDER condtionally jump over if body
            free_register(cond_reg); // can be reused
            child(1);
            label(endif);

            if (has_else) {
                label(skipelse);
                emit(JUMPF, 0, 0, 0); // PLACEHODLER jump over else body at end of if
                label(startelse);
                child(2);
                label(endelse);
                rewrite(skipelse, JUMPF, uint8_t(endelse - endif), 0, 0);
                rewrite(condjump, CONDJUMP, cond_reg, uint8_t(startelse - condjump), 0);
            } else {
                rewrite(condjump, CONDJUMP, cond_reg, uint8_t(endif - condjump), 0);
            }
            return 0xff;
        }
        handle(WhileStat) {
            label(condeval);
            auto cond_reg = child(0);
            label(condjump);
            emit(CONDJUMP, cond_reg, 0, 0);
            free_register(cond_reg);
            child(1);
            label(jumpback);
            emit(JUMPB, uint8_t(jumpback - condeval), 0, 0);
            label(endwhile);
            rewrite(condjump, CONDJUMP, cond_reg, uint8_t(endwhile - condjump), 0);
            return 0xff;
        }
        handle(VarDeclStat) {
            auto reg = child(1);
            auto new_reg = allocate_register();
            auto var = bind_register(node->children[0].data_s);
            emit(MOVE, var->reg, reg, 0); // TODO: optimize this out
            return var->reg;
        }
        handle(ConstDeclStat) {
            auto reg = child(1);
            auto new_reg = allocate_register();
            auto var = bind_register(node->children[0].data_s, true);
            emit(MOVE, var->reg, reg, 0); // TODO: optimize this out
            return var->reg;
        }
        handle(AssignStat) {
            auto source_reg = child(1);
            auto& lhs = node->children[0];
            if (lhs.type == AstType::Identifier) {
                if (auto var = lookup_var(node->children[0].data_s)) {
                    if (var->is_const) {
                        error("can't reassign const variable");
                    }
                    if (source_reg == var->reg) {
                        // writing a variable to itself? nah
                    }
                    if (var->is_box) {
                        emit(WRITE_BOX, var->reg, source_reg, 0);
                    } else {
                        emit(MOVE, var->reg, source_reg, 0);
                    }
                    // TODO: try and elide this MOVE
                    // if the moved-from register is "busy" but not "bound", then the value is not used again
                    // only need MOVE if assigning directly from another variable
                    // in which case MOVE is more like COPY
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
            return source_reg; // unused
        }
        handle(Identifier) {
            if (auto v = lookup_var(node->data_s)) {
                if (v->is_box) {
                    auto reg = allocate_register();
                    emit(READ_BOX, reg, v->reg, 0);
                    return reg;
                } else {
                    auto reg = allocate_register();
                    emit(MOVE, reg, v->reg, 0); // placeholder may get rewritten as READ_BOX
                    // desperately need to elide these
                    return reg;
                }
            } else {
                error("can't find variable: " + node->data_s);
            }
            return 0xff;
        }
        handle(NumLiteral) {
            auto n = node->data_d;
            auto out = allocate_register();
            auto index = output->store_number(node->data_d);
            emit_u(LOAD_CONST, out, index);
            return out;
        }
        handle(StringLiteral) {
            auto out = allocate_register();
            auto index = output->store_string(node->data_s);
            emit_u(LOAD_CONST, out, index);
            return out;
        }
        handle(FuncLiteral) {
            // note this may create some captures and emit READ_CAPTURE
            auto index = output->store_function();
            auto func = (CodeFragment*)value_to_pointer(output->storage[index]);
            func->name = (node->children.size() == 3) ? node->children[2].data_s : "(anonymous)";
            func->name = output->name + "::" + func->name;

            // if there's an extra identifier node, it's a named function
            auto is_named = (node->children.size() == 3);

            // add a const binding if it's a named function
            // TODO: this needs to happen at the AST level, not here (which register do we return?)
            auto var = (VariableContext*)nullptr;
            if (is_named) {
                var = bind_register(node->children[2].data_s, true);
            }

            /*
            Problem:
                compile_func needs to go below is_named MOVE
                because if the function is recursive it needs
                to capture itself, so it needs to rewrite the MOVE
                as a WRITE_BOX

                however for capture-capture scenario, 
            */

            
            // allocate new closure
            auto out = allocate_register();
            emit_u(ALLOC_FUNC, out, index);

            // write to the const binding after alloc
            if (is_named) {
                emit(MOVE, var->reg, out, 0);
            }
            
            // compile
            auto compiler = Compiler {};
            compiler.compile_func(node, func, &scopes.back());

            return out;
        }
        handle(CallExp) {
            auto nargs = node->children[1].children.size();

            // compile the arguments first and remember which registers they are in
            auto arg_regs = std::vector<uint8_t> {};
            for (auto i = 0; i < nargs; i++) {
                arg_regs.emplace_back(compile(&node->children[1].children[i]));
            }
            auto func_reg = child(0); // LHS evaluates to function

            // copy arguments to top of stack in sequence
            auto end_reg = get_end_register();
            for (auto i = 0; i < nargs; i++) {
                emit(MOVE, end_reg + i + STACK_FRAME_OVERHEAD, arg_regs[i], 0);
            }

            emit(CALL, func_reg, (uint8_t)nargs, end_reg);

            // return value goes in end-reg so mark it as used
            registers[end_reg] = RegisterState::BUSY;
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
            auto in1 = child(0);
            emit(PRINT, in1, 0, 0);
            free_register(in1);
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
#undef label
#undef rewrite
#undef emit_u
#undef emit
