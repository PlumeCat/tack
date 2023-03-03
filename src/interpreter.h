#pragma once

#include <cstring>
#include <list>
#include <cassert>

#include <jlib/hash_map.h>
#include <jlib/swiss_vector.h>

#include "value.h"
#include "instructions.h"
#include "compiler.h"

/*
TODO: keep a list of errors when encountered and attempt to continue compilation instead of throwing
     - variable missing
     - variable already exists
*/

struct Interpreter {
    Interpreter() {}
    
    void execute(Program& program) {
        // TODO: optimization, computed goto, direct threading, ...?
        
        // program counter
        auto instruction_pointer = 0;

        // stack
        const auto STACK_SIZE = 1024;
        auto stack = array<Value, STACK_SIZE>{}; memset((void*)stack.data(), 0xffffffff, stack.size() * sizeof(Value));
        auto stackptr = 0;
        auto stackbase = 0;

        // heap
        auto heap = swiss_vector<Value>((size_t)1024); // where variables live (unless stack allocated)
        
        // allocated types
        auto all_strings = swiss_vector<StringType>((size_t)1024);
        auto all_objects = swiss_vector<ObjectType>((size_t)1024);
        auto all_arrays = swiss_vector<ArrayType>((size_t)1024);

        // some stack manipulation
        auto stack_push = [&](Value v) {
            stack[stackptr++] = v;
        };
        auto stack_pop = [&]() -> Value {
            return stack[--stackptr];
        };
        auto error = [&](const string& s) {
            throw runtime_error("runtime error: " + s);
        };
        auto print_value = [&](Value v) {
            log<true, false>("print: ", v);
        };

        // HACK: sort of.
        // the top level block of the program counts as the 0th stack frame
        // so when it RETs, it needs to jump to the final instruction so the interpreter loop can terminate
        stack_push({ program.instructions.size() }); // base return address is end of code
        stack_push({ 0 }); // base frame ptr
        stackbase = 2;

        while (instruction_pointer < program.instructions.size()) {
            // decode
            auto opcode = Opcode{};
            auto r1 = uint16_t{0};
            decode_instruction(program.instructions[instruction_pointer], opcode, r1);

            #define check(index, type)  (value_get_type(stack[index]) == Type::type)
            #define echeck(index, type, msg) if (!check(index, type)) { auto s = stringstream{}; s << "invalid types: " msg "; "; s << stack[index]; error(s.str()); }
            #define handle(c)           break; case Opcode::c:
            #define handle_binop(c, cast, op) handle(c) {\
                    if (check(stackptr-2, Number) && check(stackptr-1, Number)) {\
                        stack[stackptr--] = { .d = (double)((cast)stack[stackptr-2].d op (cast)stack[stackptr-1].d) };\
                    } else { error("invalid types for operation"); }}
                //stack[stackptr-2] = stack[stackptr-2] op stack[stackptr-1]; stackptr--;
                //}

            // TODO: clean up interaction between CALL and GROW
            // CALL is muching the stack pointer / stack size, then GROW mucks it again,
            // then will need more mucking to handle args and return
            // maybe CALL / RET need to do more work, or store function metadata somewhere?
                // could put function fixed stack growth into storage after the start address?
            // execute
            switch (opcode) {
                case Opcode::UNKNOWN:
                // binop
                handle_binop(ADD,       double,     +)
                handle_binop(SUB,       double,     -)
                handle_binop(MUL,       double,     *)
                handle_binop(DIV,       double,     /)
                handle_binop(EQUAL,     double,     ==)
                handle_binop(NEQUAL,    double,     !=)
                handle_binop(LESS,      double,     <)
                handle_binop(GREATER,   double,     >)
                handle_binop(LESSEQ,    double,     <=)
                handle_binop(GREATEREQ, double,     >=)
                handle_binop(MOD,       uint64_t,   %)

                // string concat
                handle(BITAND)          { auto& s1 = *value_to_string(stack[stackptr-2]);
                                          auto& s2 = *value_to_string(stack[stackptr-1]);
                                          stack[stackptr--] = value_from_string(all_strings.index_to_pointer(all_strings.add(s1 + s2)));
                                        }
                // unop
                handle(NEGATE)          { echeck(stackptr-1, Number, "unary negate"); stack[stackptr-1] = { .d = -stack[stackptr-1].d }; }
                handle(LEN)             { echeck(stackptr-1, Array, "array len");     stack[stackptr-1] = value_from_integer(value_to_array(stack[stackptr-1])->size()); }
                handle(SHL)             { echeck(stackptr-2, Array, "array append");  value_to_array(stack[stackptr-2])->emplace_back(stack_pop()); }

                handle(GROW)            { stackptr += r1;
                                          for (auto i = 0; i < r1; i++) {
                                            auto _n = heap.add(value_null());
                                            auto _p = heap.index_to_pointer(_n);
                                            stack[stackbase+i] = value_from_boxed(_p);
                                          }
                                        }
                handle(PUSH)            { stack_push(stack[stackptr - (r1 + 1)]); }
                handle(READ_VAR)        {
                    assert((stack[stackbase+r1].i & type_bits == type_bits_boxed, "R VAR"));
                    stack_push(*value_to_box(stack[stackbase+r1]));
                }
                handle(WRITE_VAR)       {
                    assert((stack[stackbase+r1].i & type_bits == type_bits_boxed, "W VAR"));
                    // auto p = stack_pop();
                    *value_to_box(stack[stackbase+r1]) = stack_pop();
                    // print_value(p);
                }
                handle(LOAD_CONST)      { stack_push(program.storage[r1]); }
                handle(LOAD_STRING)     { stack_push(value_from_string(&program.strings[r1])); /* TODO: allow more than 2**16 strings in the program */ }
                handle(LOAD_ARRAY)      {
                    echeck(stackptr-1, Integer, "expected integer index");
                    auto index = value_to_integer(stack_pop());
                    echeck(stackptr-1, Array, "expected array");
                    auto arr = value_to_array(stack_pop());
                    stack_push(arr->data()[index]);
                }
                handle(LOAD_OBJECT)     {
                    // auto key = stack_pop();
                    // auto object = stack_pop();
                    // stack_push(object_heap[obj][key]);
                }
                
                // handle(STORE)           { stack[stackbase+r1] = stack_pop(); }
                handle(STORE_ARRAY)     {
                    auto value = stack_pop();
                    auto index = value_to_integer(stack_pop());
                    auto arr = value_to_array(stack_pop());
                    (*arr)[index] = value;
                }
                handle(STORE_OBJECT)    {
                    // auto value = stack_pop();
                    // auto key = stack_pop();
                    // auto obj = stack_pop();
                    // object_heap[obj][key] = value;
                }
                handle(ALLOC_ARRAY)     {
                    stack_push(value_from_array(all_arrays.index_to_pointer(all_arrays.add(ArrayType(r1)))));
                }
                handle(ALLOC_OBJECT) {
                    stack_push(value_from_object(all_objects.index_to_pointer(all_objects.add(ObjectType{}))));
                }
                
                handle(PRINT)           { print_value(stack_pop()); }
                
                handle(JUMPF)           { instruction_pointer += r1 - 1; }
                handle(JUMPB)           { instruction_pointer -= (r1 + 1); }
                handle(CONDJUMP)        { if (!value_to_boolean(stack_pop())) { instruction_pointer += r1 - 1; }}
                
                handle(PRECALL) {
                    stack_push({ .i = 0 }); // placeholder for return address
                    stack_push({ .i = (uint64_t)stackbase }); // return frame pointer
                }
                handle(CALL) {
                    auto call_addr = stack_pop().i;
                    stackbase = stackptr - r1;// arguments were already pushed
                    stack[stackbase - 2] = { .i = (uint64_t)instruction_pointer };// rewrite the return address lower down the stack
                    instruction_pointer = call_addr - 1; // -1 because we have instruction_pointer++ later; (TODO: optimize out?)
                }
                handle(RET) {
                    // TODO: figure out how to handle when there's no return value but one was expected
                    // eg 
                    // let a = fn() { return } let b = a()
                    // if nothing pushed to stack, assign to b will corrupt the stack with current implementation
                    // maybe push null if there's no retval?
                    auto retval = value_null();
                    if (r1) {
                        retval = stack_pop();
                    }
                    stackptr = stackbase;                    
                    stackbase = stack_pop().i;
                    instruction_pointer = stack_pop().i;
                    if (r1) {
                        stack_push(retval);
                    }
                }
                break; default: throw runtime_error("unknown instruction "s + to_string(opcode));
            }

            instruction_pointer += 1;

            #undef handle
            #undef handle_binop
        }
    }
};

