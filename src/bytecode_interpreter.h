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
    opcode(CALL) opcode(RET)\
    opcode(PRINT)\
    // opcode(EXIT)
    

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
hash_map<Opcode, string> opcode_to_string = { opcodes() };
#undef opcode


struct CompilerContext {
    vector<pair<const AstNode*, uint32_t>> functions; // code x storage location to put the start address
    hash_map<string, uint16_t> variables;
    uint32_t free_reg = 0;

    // variable -> register
    uint32_t lookup(const string& identifier) {
        auto iter = variables.find(identifier);
        if (iter == variables.end()) {
            return -1;
        }
        return iter->second;
    }
    // set register for variable
    void bind_register(const string& identifier, uint32_t reg) {
        variables[identifier] = reg;
    }
};


struct Program {
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

uint64_t encode_op(Opcode opcode, uint64_t i1 = 0, uint64_t i2 = 0, uint64_t i3 = 0) {
    if (i1 > UINT16_MAX || i2 > UINT16_MAX || i3 > UINT16_MAX) throw runtime_error("Out of range instruction");
    return (
        uint64_t(opcode) << 48 |
        uint64_t(i1) << 32 |
        uint64_t(i2) << 16 |
        uint64_t(i3)
    );
};
#define encode(op, ...) encode_op(Opcode::op, __VA_ARGS__)
#define emit(op, ...) program.instructions.push_back(encode(op, __VA_ARGS__));
#define rewrite(instr, op, ...) program.instructions[instr] = encode(op, __VA_ARGS__);
#define handle(type) break; case AstType::type:
#define child(n) compile_node(node.children[n], compiler, program);
#define label(name) auto name = program.instructions.size();
#define label_reg(name) auto name = compiler.free_reg - 1;
#define handle_binexp(Type, Opcode)\
    break; case AstType::Type: {\
        child(0); child(1);\
        emit(Opcode, compiler.free_reg - 2, compiler.free_reg - 1);\
        compiler.free_reg -= 1; }

struct BytecodeInterpreter {
    BytecodeInterpreter() {}

    void compile(const AstNode& module, CompilerContext& compiler, Program& program) {
        // treat the top level of the program as a function body
        // RET will be emitted for it - don't need a special EXIT instruction
        // when the program is entered, the 0th stack frame has the last instruction+1
        // as the return address, so the interpreter loop can terminate
        // (see 'execute')
        auto node = AstNode(
            AstType::FuncLiteral,
            AstNode(AstType::ParamDef),
            module // parsing ensures that module is always a stat-list
        );

        program.storage.emplace_back(0); // dummy address of top level body
        compiler.functions.emplace_back(pair { &node, program.storage.size() });

        // compile all function literals and append to main bytecode
        // TODO: interesting, can this be parallelized?
        for (auto i = 0; i < compiler.functions.size(); i++) { // can't use range-based for because compiler.functions size changes
            auto func = compiler.functions[i].first;
            auto storage = compiler.functions[i].second;
            program.storage[storage] = program.instructions.size();
            compile_node(func->children[1], compiler, program);
            emit(RET, 0);
        }

    }

    void compile_node(const AstNode& node, CompilerContext& compiler, Program& program) {
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
                if (compiler.lookup(ident) != -1) {
                    throw runtime_error("Compile error: variable already exists: " + ident);
                }
                compiler.bind_register(ident, compiler.free_reg - 1); // top register belongs to the variable now
                compiler.free_reg++;
            }
            handle(AssignStat) {
                child(1);
                auto reg = compiler.lookup(node.children[0].data_s);
                if (reg == -1) {
                    throw runtime_error("Compile error: variable not found: " + node.children[0].data_s);
                }
                emit(MOV, compiler.free_reg - 1, reg);
                compiler.free_reg--; // subsequent instructions can use this register
            }
            handle(PrintStat) {
                child(0);
                emit(PRINT, compiler.free_reg - 1);
                compiler.free_reg--;
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
                
                // TODO: make more efficient
                // eg don't need to jump to startif as it's the next instruction anyway
                // condjump probably just needs jump-or-don't-jump
                // also can be more efficient for the case when there's no else (probably)
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
            
            handle(NegateExp) { child(0); emit(NEGATE, compiler.free_reg - 1); }
            handle(NotExp)    { child(0); emit(NOT, compiler.free_reg - 1); }
            handle(BitNotExp) { child(0); emit(BITNOT, compiler.free_reg - 1); }

            handle(ReturnStat) {
                // TODO:
                // 'return' will work by jumping to the end of the function body
                // the end of the function has the JUMP back to return address
                // that way it's easy to support multi-return, early-return etc
                // jump folding will optimize this out hopefully
                
                // emit(JUMP, end_of_func);
            }
            handle(CallExp) {
                // TODO: indirect call vs direct call
                // If the LHS is an identifier, then it will be possible to get the address of the function at call time
                // so skip the roundtrip through function index -> storage -> register -> call
                // can do the index lookup at compile time maybe?

                // The LHS of the call expression should evaluate to the storage location
                // of a function address
                child(0);
                
                // call function
                emit(CALL, compiler.free_reg - 1);
                compiler.free_reg--;
            }
            handle(ArgList) {}
            handle(IndexExp) {}
            handle(AccessExp) {}
            handle(FuncLiteral) {
                // put a placeholder value in storage at first. when the program is linked, the placeholder is
                // overwritten with the real start address
                auto func_storage = program.storage.size();
                compiler.functions.emplace_back(pair { &node, func_storage }); // compile function
                program.storage.push_back(compiler.functions.size() * 111); // placeholder value

                // load the function start address
                emit(LOAD, compiler.free_reg, func_storage);
                compiler.free_reg++;
            }
            handle(ParamDef) {}
            handle(NumLiteral) {
                // TODO: don't store the same constants twice
                program.storage.push_back(node.data_d);
                emit(LOAD, compiler.free_reg, program.storage.size() - 1);
                compiler.free_reg++;
            }
            handle(StringLiteral) {}
            handle(ArrayLiteral) {}
            handle(ObjectLiteral) {}
            handle(Identifier) {
                // reading a variable (writing is not encountered by recursive compile)
                // so the variable exists and points to a register
                auto reg = compiler.lookup(node.data_s);
                if (reg == -1) {
                    throw runtime_error("Unknown variable: " + node.data_s);
                }
                emit(MOV, reg, compiler.free_reg);
                compiler.free_reg++;
            }
        }

        // TODO: constant folding
        // TODO: jump folding
        // TODO: mov folding
    }
    
    #undef encode
    #undef emit
    #undef handle
    #undef child
    #undef label
    #undef label_reg
    
    void execute(Program& program) {
        // TODO: optimization, computed goto, direct threading, ...
        auto registers = array<double, numeric_limits<uint16_t>::max()>{};
        
        // HACK: sort of.
        // the top level block of the program counts as the 0th stack frame
        // so when it RETs, it needs to jump to the final instruction so the interpreter loop can terminate
        auto call_stack = vector<uint32_t>{}; // literally just the return address
        call_stack.emplace_back(program.instructions.size());
        // TODO: use the registers for this somehow...
        
        auto instruction_pointer = 0;
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
                handle(CALL) {
                    call_stack.push_back(instruction_pointer);
                    instruction_pointer = registers[r1];
                }
                handle(RET) {
                    instruction_pointer = call_stack.back();
                    call_stack.pop_back();
                }
                handle(CONDJUMP)    instruction_pointer = registers[r1] ? r2 : r3;
                // handle(EXIT)        instruction_pointer = program.instructions.size(); // HACK
                break; default: throw runtime_error("unknown instruction "s + opcode_to_string[(Opcode)opcode]);
            }

            #undef handle
        }
    }
};

