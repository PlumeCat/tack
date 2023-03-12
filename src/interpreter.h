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
		const auto STACK_SIZE = 4096u;
		auto _stack = array<Value, STACK_SIZE>{}; memset((void*)_stack.data(), 0xffffffff, _stack.size() * sizeof(Value));
		auto _stackptr = 0;
		auto _stackbase = 0;

		// heap
		auto max_heap_env = std::getenv("STRAW_MAX_HEAP");
		auto default_heap = (char*)"4096";
		max_heap_env = max_heap_env ? max_heap_env : default_heap;
		auto max_heap = stoi(max_heap_env);
		log("Max heap: ", max_heap);
		auto _heap = swiss_vector<Value, false>((size_t)max_heap); // where variables live (unless stack allocated)
		//
		//// allocated types
		auto _strings = swiss_vector<StringType, false>((size_t)4096);
		auto _objects = swiss_vector<ObjectType, false>((size_t)4096);
		auto _arrays = swiss_vector<ArrayType, false>((size_t)4096);

		// some stack manipulation
		auto error = [&](const string& s)  -> Value {
			throw runtime_error("runtime error: " + s);
			return value_null();
		};

#define stack_pop() _stack[--_stackptr]
#define stack_push(v) if (_stackptr >= STACK_SIZE-1) { error("stack overflow!"); } _stack[_stackptr++] = v
#define stack_at(i)    _stack[(i > 0) ? i : _stackptr+i]
		// #define stack_set(i, v) _stack[(i > 0) ? i : _stackptr+i] = v

		auto alloc_box = [&]() -> Value {
			return value_from_boxed(&_heap.data()[_heap.add(value_null())]);
		};
		auto alloc_string = [&](const auto& s = "") -> Value {
			return value_from_string(&_strings.data()[_strings.add(s)]);
		};
		auto alloc_object = [&](auto size) -> Value {
			auto obj = &_objects.data()[_objects.add({})];
			for (auto i = 0; i < size; i++) {
				// pop value
				auto val = stack_pop();
				auto key = value_to_string(stack_pop());
				obj->insert(*key, val);
				// pop key                
			}
			return value_from_object(obj);
		};
		auto alloc_array = [&](auto size) -> Value {
			if (size) { // TODO: fast but a bit dangerous?
				auto arr = &_arrays.data()[_arrays.add(ArrayType(size))];
				memcpy(arr->data(), &_stack[_stackptr - size], sizeof(Value) * size);
				_stackptr -= size;
				return value_from_array(arr);
			} else {
				auto _arr = ArrayType();
				_arr.reserve(16);
				auto arr = &_arrays.data()[_arrays.add(_arr)];
				return value_from_array(arr);
			}
		};

		/*
		auto stack_push = [&](Value v) {
			if (_stackptr >= STACK_SIZE - 1) {
				error("stack overflow");
			}
			_stack[_stackptr++] = v;
		};
		auto stack_pop = [&]() -> Value {
			return _stack[--_stackptr];
		};
		auto stack_get = [&](int index) -> Value {
			return (index >= 0) ? _stack[index] : _stack[_stackptr+index];
		};
		auto stack_set = [&](int index, Value value) {
			_stack[index] = value;
		};
		*/

		auto print_value = [&](Value v) {
			log<true, false>(v);
		};
		auto stack_dump = [&] {
			log("===== STACK =====");
			for (auto i = 0; i < _stackptr; i++) {
				log<true>("  ", _stack[i]);
			}
			log("=================");
		};

		// HACK: sort of.
		// the top level block of the program counts as the 0th stack frame
		// so when it RETs, it needs to jump to the final instruction so the interpreter loop can terminate
		stack_push(value_from_function(program.instructions.size())); // base return address is end of code
		stack_push(value_from_integer(0)); // base frame ptr
		_stackbase = 2;

		while (instruction_pointer < program.instructions.size()) {
			// decode
			auto& instruction = program.instructions[instruction_pointer];
			auto opcode = instruction.opcode;
			auto r1 = instruction.operands[0];
			auto r2 = instruction.operands[1];
			auto r3 = instruction.operands[2];
			//decode_instruction(program.instructions[instruction_pointer], opcode, r1, r2, r3);

#define check(val, type)  (value_get_type(val) == Type::type)
#define handle(c)           break; case Opcode::c:
#define handle_binop(c, op, cast) handle(c) {\
                    auto r = stack_pop();\
                    auto l = stack_pop();\
                    auto lhs = check(l, Number)\
                        ? value_to_number(l)\
                        : check(l, Integer)\
                            ? (double)value_to_integer(l)\
                            : value_to_number(error("expect number or integer"));\
                    auto rhs = check(r, Number)\
                        ? value_to_number(r)\
                        : check(r, Integer)\
                            ? (double)value_to_integer(r)\
                            : value_to_number(error("expect number or integer"));\
                    stack_push(cast(lhs op rhs));}

			// TODO: clean up interaction between CALL and GROW
			// CALL is muching the stack pointer / stack size, then GROW mucks it again,
			// then will need more mucking to handle args and return
			// maybe CALL / RET need to do more work, or store function metadata somewhere?
				// could put function fixed stack growth into storage after the start address?
			// execute

			// TODO: arity checking for arguments and return values, (otherwise we risk damaging the stack)

			// TODO: leave variables that aren't captured on the stack, and use PUSH / SET to read them rather than READ_VAR / WRITE_VAR
			// in fact lets do that by default, and rename READ_VAR/WRITE_VAR to READ_BOX/WRITE_BOX
			// TODO: add a 'const' keyword
			//      then detect variables that are never reassigned and mark them as const
			//      const stuff can be captured by-value, then the copy is also marked const
			//      so it can be copied rather than boxed and copy the box
			//      will be especially important for recursive functions

			auto before_stackptr = _stackptr;

			switch (opcode) {
			case Opcode::UNKNOWN:

				handle(ADDI) {
					// convert r1 to signed 16 bit int
					auto l = stack_at(-1);
					auto lhs = check(l, Number)\
						? value_to_number(l)\
						: check(l, Integer)\
						? (double)value_to_integer(l)\
						: value_to_number(error("expect number or integer"));
					auto i = *(reinterpret_cast<int16_t*>(&r1));
					stack_at(-1) = value_from_number(lhs + i);
				}

				handle_binop(ADD, +, value_from_number)
				handle_binop(SUB, -, value_from_number)
				handle_binop(MUL, *, value_from_number)
				handle_binop(DIV, / , value_from_number)
				// handle_binop(MOD,       %,  value_from_integer)

				handle(EQUAL)  { stack_at(-2) = value_from_boolean(stack_at(-2)._i == stack_at(-1)._i); stack_pop(); }
				handle(NEQUAL) { stack_at(-2) = value_from_boolean(stack_at(-2)._i != stack_at(-1)._i); stack_pop(); }

				handle_binop(LESS, < , value_from_boolean)
				handle_binop(GREATER, > , value_from_boolean)
				handle_binop(LESSEQ, <= , value_from_boolean)
				handle_binop(GREATEREQ, >= , value_from_boolean)

					// unop
					// handle(NEGATE)          { echeck(stack_get(-1), Number, "unary negate"); stack_push(value_from_number(-value_to_number(stack_pop()))); }
				handle(LEN) {
					// echeck(stack_get(-1), Array, "array len");
					auto arr = stack_pop();
					if (!check(arr, Array)) {
						error("array len");
					}
					stack_push(value_from_integer(value_to_array(arr)->size()));
				}
				handle(SHL)             {
					auto v = stack_pop();
					auto a = stack_pop();
					if (!check(a, Array)) { error("array append"); }
					value_to_array(a)->emplace_back(v);
				}
				handle(GROW)            {
					for (auto i = 0; i < r1; i++) {
						// stack_push(alloc_box());
						stack_push(value_null());
					}
				}
				handle(READ_VAR)        {
					auto var = stack_at(_stackbase + r1);
					/*if (value_is_boxed(var)) {
						var = *value_to_box(var);
					}*/
					stack_push(var);
				}
				handle(WRITE_VAR)       {
					auto val = stack_pop();
					// auto var = stack_at(_stackbase + r1);
					//if (value_is_boxed(var)) {
						//*value_to_box(var) = val;
					//} else {
					stack_at(_stackbase + r1) = val;
					//}
				}
				handle(LOAD_CONST)      { stack_push(program.storage[r1]); }
				handle(LOAD_STRING)     { stack_push(value_from_string(&program.strings[r1])); /* TODO: allow more than 2**16 strings in the program */ }
				handle(ALLOC_ARRAY)     {
					stack_push(alloc_array(r1));
				}
				handle(LOAD_ARRAY)      {
					auto _index = stack_pop();
					auto index = check(_index, Number)
						? (int32_t)value_to_number(_index)
						: check(_index, Integer)
						? value_to_integer(_index)
						: value_to_integer(error("expect number or integer"));
					//echeck(stack_get(-1), Array, "expected array");
					auto arr = stack_pop();
					if (!check(arr, Array)) {
						error("expected array");
					}
					stack_push(value_to_array(arr)->data()[index]);
				}
				handle(STORE_ARRAY)     {
					auto value = stack_pop();
					auto index = value_to_integer(stack_pop());
					auto arr = value_to_array(stack_pop());
					(*arr)[index] = value;
				}
				handle(ALLOC_OBJECT) {
					// TODO: put key/values from stack
					stack_push(alloc_object(r1));
				}
				handle(LOAD_OBJECT)     {
					auto obj = stack_pop();
					if (!check(obj, Object)) { error("expect an object 1"); }
					// auto val = stack_pop();
					auto key = program.strings[r1];
					stack_push((*value_to_object(obj))[key]);
				}
				handle(STORE_OBJECT)    {
					auto val = stack_pop();
					auto obj = stack_pop();
					if (!check(obj, Object)) { error("expect an object"); }
					auto key = program.strings[r1];
					(*value_to_object(obj))[key] = val;
				}

				handle(RANDOM)          { stack_push(value_from_integer(rand() % 10000)); }
				handle(CLOCK)           {
					stack_push(value_from_number(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count() / 1e6));
				}
				handle(PRINT)           {
					auto v = stack_pop();
					print_value(v);
				}
				handle(DUMP)            {}

				handle(JUMPF)           { instruction_pointer += r1 - 1; }
				handle(JUMPB)           { instruction_pointer -= (r1 + 1); }
				handle(CONDJUMP)        {
					auto val = stack_pop();
					auto cond = value_to_boolean(val);
					if (!cond) {
						instruction_pointer += r1 - 1;
					}
				}

				handle(PRECALL) {
					stack_push(value_null());// placeholder for return address
					stack_push(value_from_integer(_stackbase));// return frame pointer
					// alloc spaces for arguments
					_stackptr += r1;
				}
				handle(CALL) {
					auto func = stack_pop();
					if (!value_is_function(func)) {
						error("tried to call non-function");
					}
					auto call_addr = value_to_function(func);
					_stackbase = _stackptr - r1 * 2;
					_stack[_stackbase - 2] = value_from_integer(instruction_pointer);// rewrite the return address lower down the stack

					// assign arguments
					// TODO: works but a bit messy, clean it up and simplify
					for (auto i = 0; i < r1; i++) {
						// auto param = _stack[_stackbase + r1 - (i + 1)];
						auto param_ptr = &_stack[_stackbase + r1 - (i + 1)];
						/*if (value_is_boxed(*param_ptr)) {
							param_ptr = value_to_box(*param_ptr);
						}*/
						*param_ptr = stack_pop();
					}

					instruction_pointer = call_addr - 1; // -1 because we have instruction_pointer++ later; (TODO: optimize out?)
				}
				handle(RET) {
					auto retval = value_null();
					if (r1) {
						retval = stack_pop();
					}
					_stackptr = _stackbase;
					_stackbase = value_to_integer(stack_pop());
					instruction_pointer = value_to_integer(stack_pop());
					//if (r1) {
					stack_push(retval);
					//}
				}
			break; default: error("unknown instruction "s + to_string(opcode)); break;
			}

			auto after_stackptr = _stackptr;

#undef handle
#undef handle_binop


			instruction_pointer += 1;

		}
		log("Boxes: ", _heap.count());
		log("Strings: ", _strings.count());
		log("Arrays: ", _arrays.count());
		log("Objects: ", _objects.count());
	}
};

