#pragma once


#define opcodes() \
	opcode(UNKNOWN)\
	opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
	opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
	opcode(NEGATE) opcode(NOT) opcode(BITNOT)\
	opcode(LOAD) opcode(STORE)\
	opcode(JUMP) opcode(CONDJUMP)\
	opcode(CALL) opcode(PRINT)

#define opcode(x) x,
enum class Opcode2 : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode2::x, #x },
hash_map<Opcode2, string> opcode_name = { opcodes() };
#undef opcode

struct Program2 {
	vector<Opcode> instructions;
    vector<double> stack; // variables, arguments, return addr
    vector<double> storage; // constants

    hash_map<string, double> variables; // name -> stack pos
};

struct BytecodeInterpreter2 {
    void compile(const AstNode& ast, Program2& out_program) {

		#define handle(x) break; case AstType::x:

		switch (node.type) {
		handle(StatList) {}
		handle(VarDeclStat) {}
		}
	}
    void execute(const Program2& program) {}
};