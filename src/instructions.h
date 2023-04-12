#pragma once

#include <jlib/hash_map.h>

#define opcodes() \
    opcode(UNKNOWN)\
    /* binary operations:*/\
    opcode(INCREMENT) opcode(DECREMENT) \
    opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
    opcode(EQUAL0) opcode(EQUALI) opcode(NEQUALI) opcode(GREATERI) opcode(LESSI) opcode(GREATEREQI) opcode(LESSEQI)\
    opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
    opcode(ADDI) opcode(SUBI) opcode(DIVI) opcode(MULI) opcode(MODI) opcode(POWI)\
    opcode(SHL) opcode(SHR) opcode(BITAND) opcode(BITOR) opcode(BITXOR) opcode(AND) opcode(OR)\
    \
    opcode(NEGATE) opcode(NOT) opcode(BITNOT) opcode(LEN) \
    \
    opcode(LOAD_CONST) \
    opcode(LOAD_I) \
    opcode(MOVE) \
    opcode(READ_CAPTURE)\
    opcode(READ_GLOBAL) opcode(WRITE_GLOBAL)\
    \
    /* lua allows 200 local variables; safet*/\
    /* lua can jump 1<<24 instructions */\
    opcode(FOR_INT) \
    opcode(JUMPF) opcode(JUMPB) opcode(CONDJUMP)\
    opcode(ALLOC_FUNC)\
    opcode(ALLOC_BOX) opcode(READ_BOX) opcode(WRITE_BOX)\
    opcode(ALLOC_ARRAY) opcode(LOAD_ARRAY) opcode(STORE_ARRAY) \
    opcode(ALLOC_OBJECT) opcode(LOAD_OBJECT) opcode(STORE_OBJECT) \
    opcode(PRECALL) opcode(CALL) opcode(RET)/**/\
    opcode(PRINT) opcode(CLOCK) opcode(RANDOM) \
    \
    opcode(OPCODE_MAX)
    

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
inline std::string to_string(Opcode opcode) {
    static const hash_map<Opcode, std::string> opcode_to_string = { opcodes() };
    return opcode_to_string[opcode];
}
#undef opcode

struct Instruction {
    union {
        struct { Opcode opcode; uint8_t r0, r1, r2; };
        struct { int16_t s0; int16_t s1; };
        struct { uint16_t u0; uint16_t u1; };
    };

    Instruction(): opcode(Opcode::UNKNOWN), r0(0), r1(0), r2(0) {}
    Instruction(const Instruction&) = default;
    Instruction(Instruction&&) = default;
    Instruction& operator=(const Instruction&) = default;
    Instruction& operator=(Instruction&&) = default;
    Instruction(Opcode op, uint8_t r0 = 0, uint8_t r1 = 0, uint8_t r2 = 0)
        : opcode(op), r0(r0), r1(r1), r2(r2) {}
};