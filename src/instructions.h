#pragma once

#include <unordered_map>

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
    opcode(READ_CAPTURE) opcode(SETNULL)\
    opcode(READ_GLOBAL) opcode(WRITE_GLOBAL)\
    \
    /* lua allows 200 local variables; safet*/\
    /* lua can jump 1<<24 instructions */\
    opcode(FOR_INT) opcode(FOR_ITER) opcode(FOR_ITER2) opcode(FOR_ITER_INIT) opcode(FOR_ITER_NEXT) \
    opcode(JUMPF) opcode(JUMPB) opcode(CONDSKIP)\
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
    static const std::unordered_map<Opcode, std::string> opcode_to_string = { opcodes() };
    return opcode_to_string.at(opcode);
}
#undef opcode

struct Instruction {
    Opcode opcode;
    uint8_t r0;
    union {
        struct { uint8_t r1, r2; } u8;
        int16_t s1;
        uint16_t u1;
    };
};