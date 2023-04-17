#include "compiler.h"
#include "value.h"
#include "interpreter.h"
#include "parsing.h"

#define JLIB_LOG_VISUALSTUDIO
#include <jlib/log.h>

#include <list>
#include <vector>
#include <array>
#include <exception>
#include <chrono>
#include <cstring>

Interpreter::Interpreter() {
    next_globalid = 0;
    stackbase = STACK_FRAME_OVERHEAD;
    global_scope.compiler = nullptr;
    global_scope.parent_scope = nullptr;
    global_scope.is_function_scope = false;
    stack.fill(value_null());
}

uint16_t Interpreter::next_gid() {
    if (next_globalid == UINT16_MAX) {
        throw std::runtime_error("too many globals");
    }
    return next_globalid++;
}

Value Interpreter::get_global(const std::string& name) {
    auto* var = global_scope.lookup(name);
    if (!var) {
        return value_null();
    }

    return globals[var->g_id];
}

Compiler::VariableContext* Interpreter::set_global(const std::string& name, bool is_const, Value value) {
    // lookup binding
    auto* var = global_scope.lookup(name);

    // create binding if not exist
    if (!var) {
        // HACK:
        var = &global_scope.bindings.try_emplace(name, Compiler::VariableContext {
            .g_id = next_gid(),
            .is_const = is_const,
            .is_global = true,
            .reg = 0xff
        }).first->second;
    }

    // size check
    if (globals.size() <= var->g_id) {
        globals.resize(var->g_id + 1);
    }

    // write initial value (useful for C; when a compiler does this, it will emit WRITE_GLOBAL with the true value)
    globals[var->g_id] = value;

    return var;
}

CodeFragment* Interpreter::create_fragment() {
    return &fragments.emplace_back();
}

Value Interpreter::load(const std::string& source) {
    auto out_ast = AstNode {};
    parse(source, out_ast);
    auto ast = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef), out_ast);

    // creates the root fragment
    auto compiler = Compiler { .interpreter = this };
    auto* fragment = create_fragment();
    fragment->name = "[global]";
    compiler.compile_func(&ast, fragment, &global_scope);
    auto* func = heap.alloc_function(fragment);

    // func is unreachable - addref it so it won't get deleted
    func->refcount = 1;
    return value_from_function(func);
}

#define handle(opcode) break; case Opcode::opcode:
#define REGISTER_RAW(n) stack[stackbase+n]
#define REGISTER(n)     *(value_is_boxed(REGISTER_RAW(n)) ? &value_to_boxed(REGISTER_RAW(n))->value : &REGISTER_RAW(n))
#define error(err) ({ throw std::runtime_error(err), value_null(); });

Value Interpreter::call(Value fn, Value* args, int nargs) {
    auto* func = value_to_function(fn);

    // program counter
    auto _pr = func; // current program
    auto _pc = 0u;
    auto _pe = _pr->bytecode->instructions.size();

    // stack/registers
    auto initial_stackbase = stackbase;
    stackbase += STACK_FRAME_OVERHEAD;

    REGISTER_RAW(-3)._i = _pr->bytecode->instructions.size();
    REGISTER_RAW(-2)._p = (void*)func;
    REGISTER_RAW(-1)._i = 0; // special case
    // stack[0]._i = _pr->bytecode->instructions.size(); // pseudo return address
    // stack[1]._p = (void*)func; // pseudo return program
    // stack[2]._i = stackbase; // reset stack base to 0, should not be reached

    auto dumpstack = [&]() {
        log("in", _pr->bytecode->name);

        auto s = stackbase;
        auto ret_s = stack[s-2]._i;
        while (stack[s-2]._i != 0) {
            auto func = (CodeFragment*)stack[s-3]._p;
            log(" -", func->name);
            s = stack[s-2]._i;
        }
    };

    try {
        while (_pc < _pe) {
            auto i = _pr->bytecode->instructions[_pc];
            switch (i.opcode) {
            case Opcode::UNKNOWN: break;
                handle(LOAD_I) {
                    REGISTER(i.r0) = value_from_number(i.u1);
                }
                handle(LOAD_CONST) {
                    REGISTER(i.r0) = _pr->bytecode->storage[i.u1];
                }
                handle(ADD) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) + value_to_number(REGISTER(i.r2)));
                }
                handle(INCREMENT) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r0)) + 1);
                }
                handle(MOD) {
                    auto x = value_to_number(REGISTER(i.r1));
                    auto y = value_to_number(REGISTER(i.r2));
                    REGISTER(i.r0) = value_from_number(fmod(x, y));
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
                handle(SHL) {
                    auto* arr = value_to_array(REGISTER(i.r0));
                    arr->values.emplace_back(REGISTER(i.r1));
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
                handle(MOVE) {
                    REGISTER(i.r0) = REGISTER(i.r1);
                }
                handle(READ_GLOBAL) {
                    REGISTER(i.r0) = globals[i.u1]; // i.u1 will never be out of range
                }
                handle(WRITE_GLOBAL) {
                    globals[i.u1] = REGISTER(i.r0); // i.u1 will never be out of range
                }
                handle(FOR_INT) {
                    auto var = value_to_number(REGISTER(i.r0));
                    auto end = value_to_number(REGISTER(i.r1));
                    if (var < end) {
                        _pc++;
                    }
                }
                handle(FOR_ITER_INIT) {
                    auto iter_val = REGISTER(i.r1);
                    auto iter_type = value_get_type(iter_val);
                    if (iter_type == Type::Array) {
                        REGISTER_RAW(i.r0)._i = 0;
                    } else if (iter_type == Type::Object) {
                        REGISTER_RAW(i.r0)._i = value_to_object(iter_val)->begin();
                    // } else if (iter_type == Type::Function) {
                    } else {
                        error("for loop expected array or object");
                    }
                }
                handle(FOR_ITER) {
                    auto iter_val = REGISTER(i.r1);
                    auto iter_type = value_get_type(iter_val);
                    if (iter_type == Type::Array) {
                        auto ind = REGISTER_RAW(i.r0)._i;
                        auto arr = value_to_array(iter_val);
                        if (ind < arr->values.size()) {
                            REGISTER(i.r2) = arr->values[ind];
                            _pc++;
                        }
                    } else if (iter_type == Type::Object) {
                        auto it = REGISTER_RAW(i.r0)._i;
                        auto obj = value_to_object(iter_val);
                        if (it != obj->end()) {
                            REGISTER(i.r2) = value_from_string(obj->key(it));
                            _pc++;
                        }
                    // } else if (iter_type == Type::Function) {
                    }
                }
                handle(FOR_ITER2) {
                    auto iter_val = REGISTER(i.r1);
                    auto obj = value_to_object(iter_val);
                    auto it = REGISTER_RAW(i.r0)._i;
                    if (it != obj->end()) {
                        REGISTER(i.r2) = value_from_string(obj->key(it));
                        REGISTER(i.r2 + 1) = obj->value(it);
                        _pc++;
                    }
                }
                handle(FOR_ITER_NEXT) {
                    auto iter_val = REGISTER(i.r1);
                    auto iter_type = value_get_type(iter_val);
                    if (iter_type == Type::Array) {
                        // increment index
                        REGISTER_RAW(i.r0)._i += 1;
                    } else if (iter_type == Type::Object) {
                        // next key
                        auto obj = value_to_object(iter_val);
                        REGISTER_RAW(i.r0)._i = obj->next(REGISTER_RAW(i.r0)._i);
                    }
                }
                handle(CONDSKIP) {
                    auto val = REGISTER(i.r0);
                    if ((
                        (value_get_type(val) == Type::Boolean && value_to_boolean(val)) ||
                        // (value_get_type(val) == Type::Integer && value_to_integer(val)) ||
                        (value_get_type(val) == Type::Number && value_to_number(val))
                        )) {
                        _pc++;
                    }
                }
                handle(JUMPF) { _pc += i.u1 - 1; }
                handle(JUMPB) { _pc -= i.u1 + 1; }
                handle(LEN) {
                    auto* arr = value_to_array(REGISTER(i.r1));
                    REGISTER(i.r0) = value_from_number(arr->values.size());
                }
                handle(READ_CAPTURE) {
                    REGISTER_RAW(i.r0) = _pr->captures[i.r1];//((Value*)(REGISTER_RAW(-1)._p))[i.r1];
                    // value_to_array(REGISTER_RAW(-1))->values.at(i.r1);
                }
                handle(ALLOC_FUNC) {
                    // create closure
                    auto code = (CodeFragment*)value_to_pointer(_pr->bytecode->storage[i.u1]);
                    auto* func = heap.alloc_function(code);

                    // capture captures
                    // box new captures (TODO: does this work?)
                    for (auto c : code->capture_info) {
                        auto val = REGISTER_RAW(c.source_register);
                        if (!value_is_boxed(val)) {
                            auto box = value_from_boxed(heap.alloc_box(val));
                            func->captures.emplace_back(box);
                            // box existing variable inplace
                            REGISTER_RAW(c.source_register) = box;
                        } else {
                            func->captures.emplace_back(val);
                        }
                    }

                    // done
                    REGISTER(i.r0) = value_from_function(func);
                }
                handle(ALLOC_ARRAY) {
                    auto* arr = heap.alloc_array();

                    // emplace child elements
                    for (auto e = 0; e < i.r1; e++) {
                        arr->values.emplace_back(REGISTER(i.r2 + e));
                    }

                    REGISTER(i.r0) = value_from_array(arr);
                }
                handle(ALLOC_OBJECT) {
                    auto* obj = heap.alloc_object();

                    // emplace child elements
                    for (auto e = 0; e < i.r1; e++) {
                        auto key = value_to_string(REGISTER(i.r2 + e * 2));
                        auto val = REGISTER(i.r2 + e * 2 + 1);
                        obj->set(key, val);
                    }
                    REGISTER(i.r0) = value_from_object(obj);
                }
                handle(LOAD_ARRAY) {
                    // TODO: rename this instr if it's going to be for objects as well
                    auto arr_val = REGISTER(i.r1);
                    auto ind_val = REGISTER(i.r2);
                    auto arr_type = value_get_type(arr_val);
                    if (arr_type == Type::Array) {
                        auto* arr = value_to_array(arr_val);
                        auto ind = value_to_number(ind_val);
                        if (ind >= arr->values.size()) {
                            error("outof range index");
                        }
                        REGISTER(i.r0) = arr->values[ind];
                    } else if (arr_type == Type::Object) {
                        auto* obj = value_to_object(arr_val);
                        auto key = value_to_string(ind_val);
                        auto found = false;
                        auto val = obj->get(key, found);
                        if (!found) {
                            error("key not found");
                        }
                        REGISTER(i.r0) = val;
                    }
                }
                handle(STORE_ARRAY) {
                    auto arr_val = REGISTER(i.r1);
                    if (value_get_type(arr_val) != Type::Array) {
                        error("expected array");
                    }
                    auto* arr = value_to_array(arr_val);
                    auto ind_val = REGISTER(i.r2);
                    auto ind = value_to_number(ind_val);

                    if (ind > arr->values.size()) {
                        error("outof range index");
                    }
                    arr->values[ind] = REGISTER(i.r0);
                }
                handle(LOAD_OBJECT) {
                    auto obj_val = REGISTER(i.r1);
                    auto key_val = REGISTER(i.r2);
                    auto* obj = value_to_object(obj_val);
                    if (value_get_type(key_val) != Type::String) {
                        error("expected string key");
                    }
                    auto key = value_to_string(key_val);
                    auto found = false;
                    auto val = obj->get(key, found);
                    if (!found) {
                        error("key not found");
                    }
                    REGISTER(i.r0) = val;
                }
                handle(STORE_OBJECT) {
                    auto obj_val = REGISTER(i.r1);
                    auto key_val = REGISTER(i.r2);
                    auto* obj = value_to_object(obj_val);
                    if (value_get_type(key_val) != Type::String) {
                        error("expected string key");
                    }
                    auto key = value_to_string(key_val);
                    obj->set(key, REGISTER(i.r0));
                }
                handle(CALL) {
                    auto r0 = REGISTER(i.r0);
                    if (value_is_function(r0)) {
                        auto func = value_to_function(r0);
                        auto nargs = i.r1;
                        auto new_base = i.r2 + STACK_FRAME_OVERHEAD;
                        
                        REGISTER_RAW(new_base - 3)._i = _pc; // push return addr
                        REGISTER_RAW(new_base - 2)._p = (void*)_pr; // return program
                        REGISTER_RAW(new_base - 1)._i = stackbase; // push return frameptr
                        // REGISTER_RAW(new_base - 1)._p = func->captures.data();

                        _pr = func;
                        _pc = -1; // TODO: gather all instructions into one big chunk to avoid this
                        _pe = _pr->bytecode->instructions.size();
                        stackbase = stackbase + new_base; // new stack frame
                    } else if (value_is_cfunction(r0)) {
                        auto cfunc = value_to_cfunction(r0);
                        auto nargs = i.r1;
                        auto new_base = i.r2 + STACK_FRAME_OVERHEAD;
                        REGISTER_RAW(i.r2) = cfunc(nargs, &stack[stackbase + new_base]);
                    } else {
                        error("tried to call non-function");
                    }
                }
                handle(RET) {
                    auto return_addr = REGISTER_RAW(-3);
                    auto return_func = REGISTER_RAW(-2);
                    auto return_stack = REGISTER_RAW(-1);
                    auto return_val = REGISTER(i.r1);

                    // "Clean" the stack
                    // - Must not leave any boxes in unused registers
                    // or subsequent loads to register will mistakenly write-through
                    // - Also must not leave any references so the GC can collect stuff off the stack
                    // TODO: try and elide this, or make more efficient
                    // std::cout << "### GC after func: " << _pr->bytecode->name << std::endl;
                    std::memset(stack.data() + stackbase, 0xffffffff, MAX_REGISTERS * sizeof(Value));
                    REGISTER_RAW(-3) = return_val;
                    heap.gc(globals, stack, stackbase);
                    REGISTER_RAW(-2) = value_null();
                    REGISTER_RAW(-1) = value_null();

                    _pc = return_addr._i;
                    _pr = (FunctionType*)return_func._p;
                    _pe = _pr->bytecode->instructions.size();
                    stackbase = return_stack._i;

                    if (stackbase == 0) {
                        // end of program
                        _pc = _pe;
                    }
                }
            break; default: error("unknown instruction: " + to_string(i.opcode));
            }
            _pc++;
        }
    } catch (std::exception& e) {
        // TODO: stack unwind
        log("runtime error: ", e.what());
        dumpstack();
    }
    stackbase = initial_stackbase;
}

#undef error
#undef handle
#undef REGISTER
#undef REGISTER_RAW
