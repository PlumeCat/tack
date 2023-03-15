#pragma once

#define opcodes() \
    opcode(UNKNOWN)\
    /* binary operations:*/\
    opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
    opcode(EQUAL0) opcode(EQUALI) opcode(NEQUALI) opcode(GREATERI) opcode(LESSI) opcode(GREATEREQI) opcode(LESSEQI)\
    opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
    opcode(ADDI) opcode(SUBI) opcode(DIVI) opcode(MULI) opcode(MODI) opcode(POWI)\
    opcode(SHL) opcode(SHR) opcode(BITAND) opcode(BITOR) opcode(BITXOR) opcode(AND) opcode(OR)\
    \
    opcode(NEGATE) opcode(NOT) opcode(BITNOT) opcode(LEN) \
    \
    opcode(LOAD_CONST) \
    opcode(MOVE)\
    \
    /* lua allows 200 local variables; safet*/\
    /* lua can jump 1<<24 instructions */\
    opcode(JUMPF) opcode(JUMPB) opcode(CONDJUMP)\
    opcode(ALLOC_OBJECT)\
    opcode(ALLOC_ARRAY) opcode(LOAD_ARRAY) \
    opcode(PRECALL) opcode(CALL) opcode(RET)/**/\
    opcode(PRINT) opcode(CLOCK) opcode(RANDOM) \
    \
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
    union {
        uint8_t operands[3];
        struct { uint8_t r0, r1, r2; };
    };

    Instruction() {}
    Instruction(const Instruction&) = default;
    Instruction(Instruction&&) = default;
    Instruction& operator=(const Instruction&) = default;
    Instruction& operator=(Instruction&&) = default;
    Instruction(Opcode op, uint8_t r1 = 0, uint8_t r2 = 0, uint8_t r3 = 0)
        : opcode(op), operands { r1, r2, r3 } {}
};