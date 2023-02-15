#pragma once


// Each instruction has up to 3 arguments
// CONDJUMP r0 i0 i1 
// - if r0 is true, jump to i0, otherwise jump to i1
// TODO: condjump i0 and i1 should be more than 16bit
// can mitigate by making it a relative jump (?) but still problematic
#include <jlib/hash_map.h>

#define opcodes() \
    opcode(UNKNOWN)\
    opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
    opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
    opcode(SHL) opcode(SHR)\
    opcode(NEGATE) opcode(NOT) opcode(BITNOT)\
    opcode(LOAD) opcode(STORE) opcode(MOV) \
    opcode(JUMP) opcode(CONDJUMP)\
    opcode(CALL) opcode(PRINT)\
    

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
hash_map<Opcode, string> opcode_to_string = { opcodes() };
#undef opcode

struct Program {
    uint32_t free_reg = 0;
    hash_map<string, uint16_t> variables;

    vector<uint64_t> instructions;
    vector<double> storage; // program constant storage goes at the bottom of the stack for now

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
        for (auto& x : storage) {
            s << x << ", ";
        }
        s << "]\n";

        s << "  Variables: {\n";
        for (auto& [ k, v]: variables) {
            s << "    " << k << ": " << v << "\n";
        }
        s << "}\n";

        return s.str();
    }
};

/*

TODO: keep a list of errors when encountered and attempt to continue compilation instead of throwing
     - variable missing
     - variable already exists

TODO: mov folding
    mov followed by usage can be elided
    eg MOV 0 1 ADD 1 2
        -> ADD 0 2
    same applies for mov chains
    eg MOV 0 1, MOV 1, 2
        -> MOV 0, 2

*/
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
        // name -> stackpos
        auto lookup = [&](const string& ident) -> uint32_t {
            auto iter = program.variables.find(ident);
            log("lookup: ", ident, iter == program.variables.end() ? "not found" : to_string(iter->second));
            if (iter == program.variables.end()) {
                return -1;
            }
            return iter->second;
        };
        auto assign = [&](const string& ident, uint32_t value) {
            log("assign: ", ident, value);
            program.variables[ident] = value;
        };
        #define encode(op, ...) _encode(Opcode::op, __VA_ARGS__)
        #define emit(op, ...) program.instructions.push_back(encode(op, __VA_ARGS__));
        #define rewrite(instr, op, ...) program.instructions[instr] = encode(op, __VA_ARGS__);
        #define handle(type) break; case AstType::type:
        #define child(n) compile(node.children[n], program);
        #define label(name) auto name = program.instructions.size();
        #define label_reg(name) auto name = program.free_reg - 1;
        #define handle_binexp(Type, Opcode)\
            break; case AstType::Type: {\
                child(0); child(1);\
                emit(Opcode, program.free_reg - 2, program.free_reg - 1);\
                program.free_reg -= 1; }

        switch (node.type) {
            case AstType::Unknown: return;
            
            handle(StatList) {
                for (auto i = 0; i < node.children.size(); i++) {
                    child(i);
                }
            }
            handle(VarDeclStat) {
                child(1); // push result of expression to free register
                
                // find the variable
                auto& ident = node.children[0].data_s;
                if (lookup(ident) != -1) {
                    throw runtime_error("Compile error: variable already exists: " + ident);
                }
                assign(ident, program.free_reg - 1); // top register belongs to the variable now
                program.free_reg++;
            }
            handle(AssignStat) {
                child(1);
                auto reg = lookup(node.children[0].data_s);
                if (reg == -1) {
                    throw runtime_error("Compile error: variable not found: " + node.children[0].data_s);
                }
                emit(MOV, program.free_reg - 1, reg);
                program.free_reg--; // subsequent instructions can use this register
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
                child(1); // compile the if body
                label(endif);
                emit(JUMP, 0, 0, 0); // PLACEHODLER

                label(startelse);
                if (node.children.size() == 3) {
                    child(2); // compile the else body
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
            handle(ReturnStat) {
                // populate the return register
                // jump to the return address
            }
            
            handle(TernaryExp) {}
            handle(OrExp) {}
            handle(AndExp) {}
            handle(BitOrExp) {}
            handle(BitAndExp) {}
            handle(BitXorExp) {}
            handle(ShiftLeftExp) {}
            handle(ShiftRightExp) {}
            
            handle_binexp(EqExp, EQUAL)
            handle_binexp(NotEqExp, NEQUAL)
            handle_binexp(LessExp, LESS)
            handle_binexp(GreaterExp, GREATER)
            handle_binexp(LessEqExp, LESSEQ)
            handle_binexp(GreaterEqExp, GREATEREQ)
            handle_binexp(AddExp, ADD)
            handle_binexp(SubExp, SUB)
            handle_binexp(MulExp, MUL)
            handle_binexp(DivExp, DIV)
            handle_binexp(ModExp, MOD)
            handle_binexp(PowExp, POW)
            
            handle(NegateExp) { child(0); emit(NEGATE, program.free_reg - 1); }
            handle(NotExp)    { child(0); emit(NOT, program.free_reg - 1); }
            handle(BitNotExp) { child(0); emit(BITNOT, program.free_reg - 1); }

            handle(CallExp) {
                /*
                label call site
                push arg
                push arg
                ...
                push return address: call site + 1
                jump
                -> return jumps here. retval is free register
                */
            }
            handle(ArgList) {}
            handle(IndexExp) {}
            handle(AccessExp) {}
            handle(FuncLiteral) {
                // Temporary
                program.storage.push_back(0);
                emit(LOAD, program.free_reg, program.storage.size() - 1);
                program.free_reg++;

                /*
                call site:
                    push arg
                    push arg
                    ...
                    push arg
                    push return
                    jump
                    pop retval

                function literal:
                    <create new stack frame>
                    
                    pop arg -> variable
                    pop arg -> variable
                    ...
                    pop arg -> variable
                    
                    <function body>
                    
                    return: 
                    push retval
                */

                
                // TODO: compile 
                // auto subprogram = Program {};
                /*compile(node.children[2], subprogram);
                program.functions.push_back(subprogram);
                program.storage.push_back(program.functions.size());
                emit(LOAD, program.free_reg, )*/
            }
            handle(ParamDef) {}
            handle(NumLiteral) {
                // TODO: don't store the same constants twice
                program.storage.push_back(node.data_d);
                emit(LOAD, program.free_reg, program.storage.size() - 1);
                program.free_reg++;
            }
            handle(StringLiteral) {}
            handle(ArrayLiteral) {}
            handle(ObjectLiteral) {}
            handle(Identifier) {
                // reading a variable (writing is not encountered by recursive compile)
                // so the variable exists and points to a register
                auto reg = lookup(node.data_s);
                if (reg == -1) {
                    throw runtime_error("Unknown variable: " + node.data_s);
                }
                emit(MOV, reg, program.free_reg);
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
        // TODO: optimization, computed goto, direct threading, ...

        struct ExecutionStackFrame {
            // TODO: maximum of 256 registers, then instructions can go back to 32bit
            // implement spilling by storing the LRU register on the stack
            //  - when a new register is needed and none are free, move contents of LRU register to stack
            //  - label it somehow so we know to reload it later
            // in fact once we have spilling, we can 
        };
        auto registers = array<double, numeric_limits<uint16_t>::max()>{};
        //vector<ExecutionStackFrame> stack;
        auto instruction_pointer = 0;
        
        // TODO: is this an acceptable way of detecting end of program, or do we need a special EXIT instruction maybe?
        while (instruction_pointer < program.instructions.size()) {
            // decode
            auto instruction = program.instructions[instruction_pointer++];
            auto opcode = uint16_t(instruction >> 48);
            auto r1 = uint16_t(instruction >> 32);
            auto r2 = uint16_t(instruction >> 16);
            auto r3 = uint16_t(instruction);

            #define handle(c) break; case (uint16_t)Opcode::c:
            
            // execute
            switch (opcode) {
                case (uint16_t)Opcode::UNKNOWN:
                handle(ADD)         registers[r1] = registers[r1] + registers[r2];
                handle(SUB)         registers[r1] = registers[r1] - registers[r2];
                handle(MUL)         registers[r1] = registers[r1] * registers[r2];
                handle(DIV)         registers[r1] = registers[r1] / registers[r2];
                handle(MOD)         registers[r1] = (double)(((uint64_t)registers[r1]) % ((uint64_t)registers[r2]));
                
                handle(EQUAL)       registers[r1] = registers[r1] == registers[r2];
                handle(NEQUAL)      registers[r1] = registers[r1] != registers[r2];
                handle(LESS)        registers[r1] = registers[r1] < registers[r2];
                handle(GREATER)     registers[r1] = registers[r1] > registers[r2];
                handle(LESSEQ)      registers[r1] = registers[r1] <= registers[r2];
                handle(GREATEREQ)   registers[r1] = registers[r1] >= registers[r2];
                
                handle(NEGATE)      registers[r1] = -registers[r1];
                
                handle(MOV)         registers[r2] = registers[r1];
                handle(LOAD)        registers[r1] = program.storage[r2];
                //handle(STORE)       program.storage[r2] = registers[r1];
                
                handle(PRINT)       log<true, false>("(print_bc)", registers[r1]);
                handle(JUMP)        instruction_pointer = r1;
                handle(CONDJUMP)    instruction_pointer = registers[r1] ? r2 : r3;
                break; default: throw runtime_error("unknown instruction "s + opcode_to_string[(Opcode)opcode]);
            }

            #undef handle
        }
    }
};

