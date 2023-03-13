#pragma once

#include  <jlib/swiss_vector.h>
#include "program.h"
#include "value.h"


// register-based single 
struct Compiler2 {
    enum RegisterState {
        FREE = 0,
        BUSY = 1,
        BOUND = 2
    };

    static const uint32_t MAX_REGISTERS = 256;
    hash_map<string, uint8_t> bindings;
    array<RegisterState, MAX_REGISTERS> registers;

    // get a register
    uint8_t allocate_register() {
        // TODO: inefficient
        for (auto i = 0; i < MAX_REGISTERS; i++) {
            if (registers[i] == FREE) {
                registers[i] = BUSY;
                return i;
            }
        }
        throw runtime_error("Ran out of registers!");
    }

    void bind_register(const string& binding, uint8_t reg) {
        registers[reg] = BOUND;
        bindings[binding] = reg;
    }
    
    // free the register if it's not bound
    void free_register(uint8_t reg) {
        if (registers[reg] != BOUND) {
            registers[reg] = FREE;
        }
    }

    // free all unbound registers
    void free_all_registers() {
        for (auto i = 0; i < MAX_REGISTERS; i++) {
            free_register(i);
        }
    }

    uint8_t compile_node(const AstNode& node, Program& out_program) {
        #define handle(x) break; case AstType::x:
        #define emit(type, r0, r1, r2) out_program.instructions.emplace_back(Opcode::type, r0, r1, r2)
        switch (node.type) {
            case AstType::Unknown: {} break;

            handle(StatList) {
                for (auto& c: node.children) {
                    compile_node(c, out_program);
                    free_all_registers();
                }
            }
            handle(VarDeclStat) {
                auto reg = compile_node(node.children[1], out_program);
                bind_register(node.children[0].data_s, reg);
                return reg;
            }
            handle(Identifier) { // read variable - returns the register used
                return bindings[node.data_s];
            }
            handle(NumLiteral) { // 1 out via LOAD_CONST
                auto in1 = (uint8_t)out_program.storage.size();
                out_program.storage.emplace_back(value_from_number(node.data_d));
                auto out = allocate_register(); // allocate a register for constant
                emit(LOAD_CONST, out, in1, 0);
                return out;
            }
            handle(AddExp) { // 1 out
                auto in1 = compile_node(node.children[0], out_program);
                auto in2 = compile_node(node.children[1], out_program);
                auto out = allocate_register();
                emit(ADD, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(MulExp) { // 1 out
                auto in1 = compile_node(node.children[0], out_program);
                auto in2 = compile_node(node.children[1], out_program);
                auto out = allocate_register();
                emit(MUL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(DivExp) { // 1 out
                auto in1 = compile_node(node.children[0], out_program);
                auto in2 = compile_node(node.children[1], out_program);
                auto out = allocate_register();
                emit(DIV, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(PrintStat) { // no output
                auto in1 = compile_node(node.children[0], out_program);
                emit(PRINT, in1, 0, 0);
                free_register(in1);
                return 0xff;
            }

            default: { throw runtime_error("unknown ast; "); } break;
        }
    }

    #undef handle
    #undef emit

    // node must be StatList
    void compile(const AstNode& node, Program& out_program) {
        compile_node(node, out_program);
    }
};


struct Interpreter2 {
    array<Value, 256> registers;

    Interpreter2() {}

    #define handle(opcode) break; case Opcode::opcode:
    void execute(const Program& program) {
        auto pc = 0;
        const auto pe = program.instructions.size();
        while (pc < pe) {
            auto i = program.instructions[pc];
            switch (i.opcode) {
                case Opcode::UNKNOWN: break;
                handle(LOAD_CONST) {
                    registers[i.r0] = program.storage[i.r1];
                }
                handle(ADD) {
                    registers[i.r0] = value_from_number(value_to_number(registers[i.r1]) + value_to_number(registers[i.r2]));
                }
                handle(MUL) {
                    registers[i.r0] = value_from_number(value_to_number(registers[i.r1]) * value_to_number(registers[i.r2]));
                }
                handle(DIV) {
                    registers[i.r0] = value_from_number(value_to_number(registers[i.r1]) / value_to_number(registers[i.r2]));
                }
                handle(PRINT) {
                    log<false,false>(registers[i.r0]);
                }
                break; default: throw runtime_error("unknown instruction: " + to_string(i.opcode));
            }
            pc++;
        }
    }
};