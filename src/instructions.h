#pragma once

#include <unordered_map>
#include <string>

#define opcodes() \
    opcode(UNKNOWN)\
    /* unary operations */\
    \
    opcode(INCREMENT)\
    opcode(DECREMENT) \
    opcode(NEGATE)\
    opcode(NOT)\
    opcode(BITNOT)\
    opcode(LEN) \
    \
    /* binary operations:*/\
    opcode(EQUAL)\
    opcode(NEQUAL)\
    opcode(GREATER)\
    opcode(LESS)\
    opcode(GREATEREQ)\
    opcode(LESSEQ)\
    opcode(ADD)\
    opcode(SUB)\
    opcode(DIV)\
    opcode(MUL)\
    opcode(MOD)\
    opcode(POW)\
    opcode(SHL)\
    opcode(SHR)\
    opcode(BITAND)\
    opcode(BITOR)\
    opcode(BITXOR)\
    opcode(AND)\
    opcode(OR)\
    \
    opcode(LOAD_CONST) \
    opcode(LOAD_I_SN)\
    opcode(LOAD_I_BOOL)\
    opcode(LOAD_I_NULL) \
    opcode(MOVE) \
    opcode(READ_CAPTURE)\
    opcode(ZERO_CAPTURE)\
    opcode(READ_GLOBAL)\
    opcode(WRITE_GLOBAL)\
    \
    /* lua allows 200 local variables */\
    /* lua can jump 1<<24 instructions */\
    opcode(FOR_INT)\
    opcode(FOR_ITER)\
    opcode(FOR_ITER2)\
    opcode(FOR_ITER_INIT)\
    opcode(FOR_ITER_NEXT) \
    opcode(JUMPF)\
    opcode(JUMPB)\
    opcode(CONDSKIP)\
    opcode(ALLOC_FUNC)\
    opcode(ALLOC_BOX)\
    opcode(READ_BOX)\
    opcode(WRITE_BOX)\
    opcode(ALLOC_ARRAY)\
    opcode(LOAD_ARRAY)\
    opcode(STORE_ARRAY) \
    opcode(ALLOC_OBJECT)\
    opcode(LOAD_OBJECT)\
    opcode(STORE_OBJECT) \
    opcode(PRECALL)\
    opcode(CALL)\
    opcode(RET)\
    opcode(PRINT)\
    opcode(CLOCK)\
    opcode(RANDOM) \
    \
    opcode(OPCODE_MAX)
    

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
inline std::string to_string(Opcode opcode) {
    static const std::unordered_map<Opcode, std::string> opcodestring = { opcodes() };
    return opcodestring.at(opcode);
}
#undef opcode

struct InputRegister { uint8_t r1, r2; };
struct Instruction {
    Opcode opcode;
    uint8_t r0;
    union {
        InputRegister u8;
        int16_t s1;
        uint16_t u1;
    };
};
