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
    global_scope.compiler = nullptr;
    global_scope.parent_scope = nullptr;
    global_scope.is_function_scope = false;
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

void Interpreter::set_global(const std::string& name, Value value) {
    // lookup binding
    auto* var = global_scope.lookup(name);

    // create binding if not exist
    if (!var) {
        // HACK:
        var = &global_scope.bindings.try_emplace(name, Compiler::VariableContext {
            .g_id = next_gid(),
            .is_const = false,
            .is_global = true,
            .reg = 0xff
        }).first->second;
    }

    // size check
    if (globals.size() <= var->g_id) {
        globals.resize(var->g_id + 1);
    }

    // write
    globals[var->g_id] = value;
}

void Interpreter::execute(const std::string& code, int argc, char* argv[]) {
    auto check_arg = [&](const std::string& s) {
        for (auto i = 0; i < argc; i++) {
            if (argv[i] == s) {
                return true;
            }
        }
        return false;
    };

    auto out_ast = AstNode {};
    parse(code, out_ast);
    if (check_arg("-A")) {
        log("AST: ", out_ast.tostring());
    }

    auto global = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef), out_ast);
    auto compiler = Compiler { .interpreter = this };
    auto program = CodeFragment { .name = "[global]" };
    compiler.compile_func(&global, &program, &global_scope);
    if (check_arg("-D")) {
        log("Program:\n" + program.str());
    }
    
    execute(&program);
}

// TODO: unify this with CALL
// externally appears as if modules are loaded as functions and called immediately
void Interpreter::execute(CodeFragment* program) {
    #define handle(opcode) break; case Opcode::opcode:
    #define REGISTER_RAW(n) _stack[_stackbase+n]
    #define REGISTER(n)     (value_is_boxed(REGISTER_RAW(n)) ? value_to_boxed(REGISTER_RAW(n))->value : REGISTER_RAW(n))

    // program counter
    auto _pr = program; // current program
    auto _pc = 0u;
    auto _pe = _pr->instructions.size();

    // stack/registers
    auto _stack = StackType {};
    auto _stackbase = STACK_FRAME_OVERHEAD;
    _stack.fill(value_null());
    _stack[0]._i = _pr->instructions.size(); // pseudo return address
    _stack[1]._p = (void*)program; // pseudo return program
    _stack[2]._i = 0; // reset stack base to 0, should not be reached
    _stack[3] = value_null(); // captures array

    #define error(err) ({ throw std::runtime_error(err), value_null(); });
    auto dumpstack = [&]() {
        log("in", _pr->name);

        auto s = _stackbase;
        auto ret_s = _stack[s-2]._i;
        while (_stack[s-2]._i != 0) {
            auto func = (CodeFragment*)_stack[s-3]._p;
            log(" -", func->name);
            s = _stack[s-2]._i;
        }
    };

    try {
        while (_pc < _pe) {
            auto i = _pr->instructions[_pc];
            switch (i.opcode) {
            case Opcode::UNKNOWN: break;
                handle(LOAD_I) {
                    REGISTER(i.r0) = value_from_number(i.u1);
                }
                handle(LOAD_CONST) {
                    REGISTER(i.r0) = _pr->storage[i.u1];
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
                // TODO: Bin ops need type checking
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
                    REGISTER(i.r0) = globals[i.u1];
                }
                handle(WRITE_GLOBAL) {
                    // resize globals if necessary
                    // invalidating pointers is fine since globals should never be boxed
                    if (i.u1 >= globals.size()) {
                        globals.resize(i.u1+1);
                    }
                    globals[i.u1] = REGISTER(i.r0);
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
                handle(ALLOC_FUNC) {
                    // create closure
                    auto code = (CodeFragment*)value_to_pointer(_pr->storage[i.u1]);
                    auto* func = heap.alloc_function(code);

                    // capture captures
                    // box new captures (TODO: does this work?)
                    for (auto c : code->capture_info) {
                        auto val = REGISTER_RAW(c.source_register);
                        if (!value_is_boxed(val)) {
                            auto box = value_from_boxed(heap.alloc_box(val));
                            func->captures->values.emplace_back(box);
                            // box existing variable inplace
                            REGISTER_RAW(c.source_register) = box;
                        } else {
                            func->captures->values.emplace_back(val);
                        }
                    }

                    // done
                    REGISTER(i.r0) = value_from_function(func);
                }
                handle(READ_CAPTURE) {
                    REGISTER_RAW(i.r0) = value_to_array(REGISTER_RAW(-1))->values.at(i.r1);
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
                        
                        REGISTER_RAW(new_base - 4)._i = _pc; // push return addr
                        REGISTER_RAW(new_base - 3)._p = (void*)_pr; // return program
                        REGISTER_RAW(new_base - 2)._i = _stackbase; // push return frameptr
                        REGISTER_RAW(new_base - 1) = value_from_array(func->captures);

                        _pr = func->bytecode;
                        _pc = -1; // TODO: gather all instructions into one big chunk to avoid this
                        _pe = _pr->instructions.size();
                        _stackbase = _stackbase + new_base; // new stack frame
                    } else if (value_is_cfunction(r0)) {
                        auto cfunc = value_to_cfunction(r0);
                        auto nargs = i.r1;
                        auto new_base = i.r2 + STACK_FRAME_OVERHEAD;
                        REGISTER_RAW(i.r2) = cfunc(nargs, &_stack[_stackbase + new_base]);
                    } else {
                        error("tried to call non-function");
                    }
                }
                handle(RET) {
                    auto return_addr = REGISTER_RAW(-4);
                    auto return_func = REGISTER_RAW(-3);
                    auto return_stack = REGISTER_RAW(-2);

                    // "Clean" the stack
                    // - Must not leave any boxes in unused registers
                    // or subsequent loads to register will mistakenly write-through
                    // - Also must not leave any references so the GC can collect stuff off the stack
                    // TODO: try and elide this, or make more efficient
                    REGISTER_RAW(-4) = i.r0 ? REGISTER_RAW(i.r1) : value_null();
                    // std::cout << "### GC after func: " << _pr->name << std::endl;
                    std::memset(_stack.data() + _stackbase, 0xffffffff, MAX_REGISTERS * sizeof(Value));
                    heap.gc(globals, _stack, _stackbase);
                    REGISTER_RAW(-3) = value_null();
                    REGISTER_RAW(-2) = value_null();
                    REGISTER_RAW(-1) = value_null();

                    _pc = return_addr._i;
                    _pr = (CodeFragment*)return_func._p;
                    _pe = _pr->instructions.size();
                    _stackbase = return_stack._i;

                    if (_stackbase == 0) {
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

    #undef error
}

