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
    /*opcode(PUSH)*/ \
    opcode(READ_VAR) \
    opcode(WRITE_VAR) \
    \
    opcode(CLOCK) opcode(RANDOM) \
    opcode(LOAD_CONST) opcode(LOAD_STRING) opcode(LOAD_ARRAY) opcode(LOAD_OBJECT) opcode(STORE_ARRAY) opcode(STORE_OBJECT) \
    opcode(GROW) \
    /* Relative jump using encoded offset */\
    opcode(JUMPF) opcode(JUMPB)\
    /* Relative jump using encoded offset IFF stack[n--] == 0 (but pop the top regardless) */\
    opcode(CONDJUMP)\
    opcode(ALLOC_OBJECT) opcode(ALLOC_ARRAY)\
    opcode(PRECALL) opcode(CALL) opcode(RET)/**/\
    opcode(PRINT) \
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

using Instruction = uint32_t;

Instruction encode_instruction(Opcode opcode, uint32_t operand = 0) {
    if (operand > UINT16_MAX) {
        throw runtime_error("Out of range operand");
    }
    return Instruction(
        uint32_t(opcode) << 16 |
        uint32_t(operand)
    );
};
void decode_instruction(Instruction instruction, Opcode& out_opcode, uint16_t& out_operand) {
    out_opcode = (Opcode)(uint16_t)(instruction >> 16);
    out_operand = (uint16_t)(instruction & 0xffff);
}

