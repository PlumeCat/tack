#pragma once

#define opcodes() \
    opcode(UNKNOWN)\
    /* binary operations:*/\
    opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
    opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
    opcode(ADDI) \
    opcode(SHL) opcode(SHR) opcode(BITAND) opcode(BITOR) opcode(BITXOR) opcode(AND) opcode(OR)\
    /* unary operation: apply inplace for value at top of stack*/\
    opcode(NEGATE) opcode(NOT) opcode(BITNOT) opcode(LEN) \
    \
    /* stack and variable operations */\
    opcode(READ_VAR) \
    opcode(WRITE_VAR) \
    \
    opcode(LOAD_CONST) opcode(LOAD_STRING) \
    opcode(LOAD_ARRAY) opcode(LOAD_OBJECT) opcode(STORE_ARRAY) opcode(STORE_OBJECT) \
    opcode(GROW) \
    \
    opcode(JUMPF) opcode(JUMPB) opcode(CONDJUMP)\
    opcode(ALLOC_OBJECT) opcode(ALLOC_ARRAY)\
    opcode(PRECALL) opcode(CALL) opcode(RET)/**/\
    opcode(PRINT) opcode(CLOCK) opcode(RANDOM) \
    opcode(DUMP) /* dump stack */\
    opcode(OPCODE_MAX)
    

#include <unordered_map>
#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
string to_string(Opcode opcode) {
    static const hash_map<Opcode, string> opcode_to_string = { opcodes() };
    return opcode_to_string[opcode];
}
#undef opcode

struct Instruction {
    Opcode opcode;
    uint8_t operands[3];

    Instruction() {}
    Instruction(const Instruction&) = default;
    Instruction(Instruction&&) = default;
    Instruction& operator=(const Instruction&) = default;
    Instruction& operator=(Instruction&&) = default;
    Instruction(Opcode op, uint8_t r1 = 0, uint8_t r2 = 0, uint8_t r3 = 0)
        : opcode(op), operands { r1, r2, r3 } {}
};