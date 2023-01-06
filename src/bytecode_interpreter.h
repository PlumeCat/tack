#pragma once


// Each instruction has up to 3 arguments
// CONDJUMP r0 i0 i1 
// - if r0 is true, jump to i0, otherwise jump to i1
// TODO: condjump i0 and i1 should be more than 16bit
// can mitigate by making it a relative jump (?) but still problematic

#define opcodes() \
	opcode(UNKNOWN)\
	opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
	opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
	opcode(NEGATE) opcode(NOT) opcode(BITNOT)\
	opcode(LOAD) opcode(STORE)\
	opcode(JUMP) opcode(CONDJUMP)\
	opcode(CALL) opcode(PRINT)
	

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
template<> struct default_hash<Opcode> { uint32_t operator()(Opcode o) { return (uint32_t)o + 1; }};
hash_map<Opcode, string, default_hash<Opcode>, 0> opcode_to_string = { opcodes() };
#undef opcode

struct Program {
	uint32_t free_reg = 0;
	vector<uint64_t> instructions;
	vector<double> stack; // program constant storage goes at the bottom of the stack for now
	hash_map<string, uint8_t> variables; // name => stackpos
	vector<Program> functions; // TODO: questionable style, fix it

	string to_string() {
		auto s = stringstream {};

		s << "  Bytecode:\n";
		for (auto bc : instructions) {
			auto opcode = uint16_t(bc >> 48);
			auto r1 = uint16_t(bc >> 32);
			auto r2 = uint16_t(bc >> 16);
			auto r3 = uint16_t(bc);
			s << "    " << opcode_to_string[(Opcode)opcode] << ", " << r1 << ", " << r2 << ", " << r3 << "\n";
		}

		s << "  Storage: [ ";
		for (auto& x : stack) {
			s << x << ", ";
		}
		s << "]\n";

		return s.str();
	}
};

struct BytecodeInterpreter {
	BytecodeInterpreter() {}
	void compile(const AstNode& node, Program& program) {
		auto _encode = [](Opcode opcode, uint64_t i1 = 0, uint64_t i2 = 0, uint64_t i3 = 0) {
			if (i1 > UINT16_MAX || i2 > UINT16_MAX || i3 > UINT16_MAX) throw runtime_error("Out of range instruction");
			return (
				uint64_t(opcode) << 48 |
				uint64_t(i1) << 32 |
				uint64_t(i2) << 16 |
				uint64_t(i3)
			);
		};
		#define encode(op, ...) _encode(Opcode::op, __VA_ARGS__)
		#define emit(op, ...) program.instructions.push_back(encode(op, __VA_ARGS__));
		#define rewrite(instr, op, ...) program.instructions[instr] = encode(op, __VA_ARGS__);
		#define handle(type) break; case AstType::type:
		#define child(n) compile(node.children[n], program);
		#define label(name) auto name = program.instructions.size();
		#define label_reg(name) auto name = program.free_reg - 1;

		switch (node.type) {
			case AstType::Unknown: return;
			
			handle(StatList) {
				for (auto i = 0; i < node.children.size(); i++) {
					child(i);
				}
			}
			handle(VarDeclStat) {
				child(1); // pushes 1 value
				// auto& stackpos = program.variables[node.children[0].data_s];
				auto& ident = node.children[0].data_s;
				auto iter = program.variables.find(ident);
				if (iter != program.variables.end()) {
					throw runtime_error("Compile error: variable already exists:" + ident);
				}
				auto stackpos = program.stack.size();
				emit(STORE, program.free_reg - 1, stackpos);
				// program.variables[ident] = stackpos;
				program.variables.insert(ident, stackpos);
				program.stack.push_back(0);
				program.free_reg--;
				// create a stack entry
			}
			handle(AssignStat) {
				child(1);
				auto& ident = node.children[0].data_s;
				auto iter = program.variables.find(ident);
				if (iter == program.variables.end()) {
					throw runtime_error("Compile error: variable not found: " + ident);
				}
				emit(STORE, program.free_reg - 1, iter->second);
				program.free_reg--;
			}
			handle(PrintStat) {
				child(0);
				emit(PRINT, program.free_reg - 1);
				program.free_reg--;
			}
			handle(IfStat) {
				child(0); // evaluate the expression
				
				label(cond);
				label_reg(cond_reg);
				emit(CONDJUMP, 0, 0, 0); // PLACEHOLDER

				label(startif);
				child(1);
				label(endif);
				emit(JUMP, 0, 0, 0); // PLACEHODLER

				label(startelse);
				if (node.children.size() == 3) {
					child(2);
				}
				label(endelse);
				
				rewrite(cond, CONDJUMP, cond_reg, startif, startelse);
				rewrite(endif, JUMP, endelse);
			}
			handle(WhileStat) {
				label(eval_cond);
				child(0); // evaluate expression
				
				label(cond);
				label_reg(cond_reg);
				emit(CONDJUMP, 0, 0, 0); // placeholder

				label(startwhile);
				child(1);
				emit(JUMP, eval_cond);
				label(endwhile);

				rewrite(cond, CONDJUMP, cond_reg, startwhile, endwhile);
			}
			handle(ReturnStat) {}
			
			handle(TernaryExp) {}
			handle(OrExp) {}
			handle(AndExp) {}
			handle(BitOrExp) {}
			handle(BitAndExp) {}
			handle(BitXorExp) {}
			handle(EqExp) {
				child(0); child(1);
				emit(EQUAL, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(NotEqExp) {
				child(0); child(1);
				emit(NEQUAL, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(LessExp) {
				child(0); child(1);
				emit(LESS, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(GreaterExp) {
				child(0); child(1);
				emit(GREATER, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(LessEqExp) {
				child(0); child(1);
				emit(LESSEQ, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(GreaterEqExp) {
				child(0); child(1);
				emit(GREATEREQ, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(ShiftLeftExp) {}
			handle(ShiftRightExp) {}
			handle(AddExp) {
				child(0); child(1);
				emit(ADD, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			};
			handle(SubExp) {
				child(0); child(1);
				emit(SUB, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(MulExp) {
				child(0); child(1);
				emit(MUL, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(DivExp) {
				child(0); child(1);
				emit(DIV, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(ModExp) {
				child(0); child(1);
				emit(MOD, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}				
			handle(PowExp) {
				child(0); child(1);
				emit(POW, program.free_reg - 2, program.free_reg - 1);
				program.free_reg--;
			}
			handle(NegateExp) {
				child(0);
				emit(NEGATE, program.free_reg - 1);
			}
			handle(NotExp) {
				child(0);
				emit(NOT, program.free_reg - 1);
			}
			handle(BitNotExp) {
				child(0);
				emit(BITNOT, program.free_reg - 1);
			}

			handle(CallExp) {}
			handle(ArgList) {}
			handle(IndexExp) {}
			handle(AccessExp) {}
			handle(FuncLiteral) {
				//auto subprogram = Program{};
				/*compile(node.children[2], subprogram);
				program.functions.push_back(subprogram);
				program.stack.push_back(program.functions.size());
				emit(LOAD, program.free_reg, )*/
			}
			handle(ParamDef) {}
			handle(NumLiteral) {
				// TODO: don't store the same constants twice
				program.stack.push_back(node.data_d);
				emit(LOAD, program.free_reg, program.stack.size() - 1);
				program.free_reg++;
			}
			handle(StringLiteral) {}
			handle(ArrayLiteral) {}
			handle(ObjectLiteral) {}
			handle(Identifier) {
				// should only be encountered when compiling sub-expressions
				auto iter = program.variables.find(node.data_s);
				if (iter == program.variables.end()) {
					throw runtime_error("Unknown variable: " + node.data_s);
				}
				emit(LOAD, program.free_reg, iter->second);
				program.free_reg++;
			}
		}

		#undef encode
		#undef emit
		#undef handle
		#undef child
		#undef label
		#undef label_reg
		// TODO: constant folding
		// TODO: jump folding
	}
	void execute(Program& program) {
		// TODO: optimization, computed goto, direct threading, ssa, ...
		auto registers = array<double, numeric_limits<uint16_t>::max()>{};
		auto instruction_pointer = 0;
		while (instruction_pointer < program.instructions.size()) {
			// decode
			auto instruction = program.instructions[instruction_pointer++];
			auto opcode = uint16_t(instruction >> 48);
			auto r1 = uint16_t(instruction >> 32);
			auto r2	= uint16_t(instruction >> 16);
			auto r3 = uint16_t(instruction);

			#define handle(c) break; case (uint16_t)Opcode::c:
			
			// execute
			switch (opcode) {
				case (uint16_t)Opcode::UNKNOWN:
				handle(ADD)			registers[r1] = registers[r1] + registers[r2];
				handle(SUB)			registers[r1] = registers[r1] - registers[r2];
				handle(MUL)			registers[r1] = registers[r1] * registers[r2];
				handle(DIV)			registers[r1] = registers[r1] / registers[r2];
				handle(MOD)			registers[r1] = (double)(((uint64_t)registers[r1]) % ((uint64_t)registers[r2]));
				
				handle(EQUAL)		registers[r1] = registers[r1] == registers[r2];
				handle(NEQUAL)		registers[r1] = registers[r1] != registers[r2];
				handle(LESS)		registers[r1] = registers[r1] < registers[r2];
				handle(GREATER)		registers[r1] = registers[r1] > registers[r2];
				handle(LESSEQ)		registers[r1] = registers[r1] <= registers[r2];
				handle(GREATEREQ)	registers[r1] = registers[r1] >= registers[r2];
				
				handle(NEGATE)		registers[r1] = -registers[r1];
				
				handle(LOAD)		registers[r1] = program.stack[r2];
				handle(STORE)		program.stack[r2] = registers[r1];
				handle(PRINT)		log<true, false>("(print_bc)", registers[r1]);
				handle(JUMP)		instruction_pointer = r1;
				handle(CONDJUMP)	instruction_pointer = registers[r1] ? r2 : r3;
				break; default: throw runtime_error("unknown instruction "s + opcode_to_string[(Opcode)opcode]);
			}

			#undef handle
		}
	}
};

