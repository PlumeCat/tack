#include "compiler.h"
#include "value.h"
#include "interpreter.h"

#define JLIB_LOG_VISUALSTUDIO
#include <jlib/log.h>

#include <list>
#include <vector>
#include <array>
#include <exception>
#include <chrono>
#include <cstring>


void Interpreter::execute(CodeFragment* program) {

    // TODO: likely super inefficient
    #define handle(opcode) break; case Opcode::opcode: //log("Opcode: ", to_string(Opcode::opcode));
    #define REGISTER_RAW(n) _stack[_stackbase+n]
    #define REGISTER(n)     *(value_is_boxed(REGISTER_RAW(n)) ? value_to_boxed(REGISTER_RAW(n)) : &REGISTER_RAW(n))

    // program counter
    auto _pr = program; // current program
    auto _pc = 0u;
    auto _pe = _pr->instructions.size();

    // stack/registers
    auto _stack = std::array<Value, 4096> {};
    auto _stackbase = STACK_FRAME_OVERHEAD;
    _stack.fill(value_null());
    _stack[0]._i = _pr->instructions.size(); // pseudo return address
    _stack[1]._p = (void*)program; // pseudo return program
    _stack[2]._i = 0; // reset stack base to 0, should not be reached
    _stack[3] = value_null();

    // heap
    auto _arrays = std::list<ArrayType> {};
    auto _objects = std::list<ObjectType> {};
    auto _functions = std::list<FunctionType> {};
    auto _boxes = std::list<Value> {};
    auto error = [&](auto err) { throw std::runtime_error(err); return value_null(); };

    try {
        while (_pc < _pe) {
            auto i = _pr->instructions[_pc];
            switch (i.opcode) {
            case Opcode::UNKNOWN: break;
                handle(LOAD_I) {
                    REGISTER(i.r0) = value_from_integer(i.s1);
                }
                handle(LOAD_CONST) {
                    REGISTER(i.r0) = _pr->storage[i.r1];
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
                        )) {
                        _pc += i.r1 - 1;
                    }
                }
                handle(JUMPF) { _pc += i.r0 - 1; }
                handle(JUMPB) { _pc -= i.r0 + 1; }
                handle(LEN) {
                    auto* arr = value_to_array(REGISTER(i.r1));
                    REGISTER(i.r0) = value_from_number(arr->size());
                }
                handle(ALLOC_FUNC) {
                    // create closure
                    auto code = (CodeFragment*)value_to_pointer(_pr->storage[i.u1]);
                    _functions.emplace_back(FunctionType { code, {} });

                    // capture captures
                    // box new captures (TODO: does this work?)
                    auto func = &_functions.back();
                    for (auto c : code->capture_info) {
                        auto val = REGISTER_RAW(c.source_register);
                        if (!value_is_boxed(val)) {
                            _boxes.emplace_back(val);
                            auto box = value_from_boxed(&_boxes.back());
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
                handle(READ_CAPTURE) {
                    REGISTER_RAW(i.r0) = value_to_array(REGISTER_RAW(-1))->at(i.r1);
                }
                handle(ALLOC_ARRAY) {
                    _arrays.emplace_back(ArrayType {});

                    // emplace child elements
                    for (auto e = 0; e < i.r1; e++) {
                        _arrays.back().emplace_back(REGISTER(i.r2 + e));
                    }

                    REGISTER(i.r0) = value_from_array(&_arrays.back());
                }
                handle(ALLOC_OBJECT) {
                    _objects.emplace_back(ObjectType {});
                    // emplace child elements
                    for (auto e = 0; e < i.r1; e++) {
                        auto key = value_to_string(REGISTER(i.r2 + e * 2));
                        auto val = REGISTER(i.r2 + e * 2 + 1);
                        _objects.back()[*key] = val;
                    }
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
                    auto func = value_to_function(REGISTER(i.r0));
                    auto nargs = i.r1;
                    auto new_base = i.r2 + STACK_FRAME_OVERHEAD;
                    REGISTER_RAW(new_base - 4)._i = _pc; // push return addr
                    REGISTER_RAW(new_base - 3)._p = (void*)_pr; // return program
                    REGISTER_RAW(new_base - 2)._i = _stackbase; // push return frameptr
                    REGISTER_RAW(new_base - 1) = value_from_array(&func->captures);

                    _pr = func->bytecode;
                    _pc = -1; // TODO: gather all instructions into one big chunk to avoid this
                    _pe = _pr->instructions.size();
                    _stackbase = _stackbase + new_base; // new stack frame
                }
                handle(RANDOM) {
                    REGISTER(i.r0) = value_from_number(rand() % 10000);
                }
                handle(CLOCK) {
                    REGISTER(i.r0) = value_from_number(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1e6);
                }
                handle(RET) {
                    auto return_addr = REGISTER_RAW(-4);
                    auto return_func = REGISTER_RAW(-3);
                    auto return_stack = REGISTER_RAW(-2);

                    REGISTER_RAW(-4) = i.r0 ? REGISTER_RAW(i.r1) : value_null();
                    REGISTER_RAW(-3) = value_null();
                    REGISTER_RAW(-2) = value_null();
                    REGISTER_RAW(-1) = value_null();

                    // "Clean" the stack
                    // Must not leave any boxes in unused registers, or subsequent loads to register will mistakenly write-through
                    // TODO: try and elide this, or make more efficient
                    std::memset(_stack.data() + _stackbase, 0xffffffff, MAX_REGISTERS * sizeof(Value));

                    _pc = return_addr._i;
                    _pr = (CodeFragment*)return_func._p;
                    _pe = _pr->instructions.size();
                    _stackbase = return_stack._i;

                    if (_stackbase == 0) {
                        // end of program
                        _pc = _pe;
                    }
                }
                handle(PRINT) {
                    log<true, false>(REGISTER(i.r0));
                }
            break; default: error("unknown instruction: " + to_string(i.opcode));
            }
            _pc++;
        }
    } catch (std::exception& e) {
        // TODO: stack unwind
        log("runtime error: ", e.what());
    }
}

