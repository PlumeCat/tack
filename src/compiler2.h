#pragma once

#include <list>
#include  <jlib/swiss_vector.h>
#include "program.h"
#include "value.h"



bool is_integer(double d) {
    return trunc(d) == d;
}

bool is_small_integer(double d) {
    return is_integer(d) && 
        d <= std::numeric_limits<int8_t>::max() &&
        d >= std::numeric_limits<int8_t>::min();
}

enum RegisterState {
    FREE = 0,
    BUSY = 1,
    BOUND = 2
};


struct FunctionContext;
struct Compiler2 {
    list<FunctionContext> funcs; // need to grow without invalidating pointers

    void add_function(Program& program, const AstNode* node);
    void compile(const AstNode& module, Program& program);
};

static const uint32_t MAX_REGISTERS = 256;
struct ScopeContext {
    hash_map<string, uint8_t> bindings; // variables
};
struct FunctionContext {
    Compiler2* compiler;
    const AstNode* node;
    uint8_t storage_index;

    // vector<ScopeContext> scopes;
    hash_map<string, uint8_t> bindings;
    array<RegisterState, MAX_REGISTERS> registers;

    // get a free register
    uint8_t allocate_register() {
        // TODO: inefficient
        // TODO: lifetime analysis for varibles
        // make the register available as soon as the variable is not used any more
        for (auto i = 0; i < MAX_REGISTERS; i++) {
            if (registers[i] == FREE) {
                registers[i] = BUSY;
                return i;
            }
        }
        throw runtime_error("Ran out of registers!");
    }

    // get the register immediately after the highest non-free register
    uint8_t get_end_register() {
        for (auto i = 255; i > 0; i--) {
            if (registers[i] != FREE) {
                return i + 1;
            }
        }
    }

    // set a register as bound by a variable
    void bind_register(const string& binding, uint8_t reg) {
        registers[reg] = BOUND;
        bindings[binding] = reg;
    }
    
    // free a register if it's not bound
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

    #define handle(x) break; case AstType::x:
    #define emit(type, r0, r1, r2) program.instructions.emplace_back(Opcode::type, r0, r1, r2)
    #define emit2(...) program.instructions.emplace_back(__VA_ARGS__)

    uint8_t compile(Program& program) {
        // compile function literal
        // TODO: emit bindings for the N arguments
        auto reg = compile(node->children[1], program);
        emit(RET, 0, 0, 0);
    }
    uint8_t compile(const AstNode& node, Program& program) {
        switch (node.type) {
            case AstType::Unknown: {} break;

            // handle(StatList)
        
            // handle(ConstDeclStat)
            // handle(VarDeclStat)
            // handle(AssignStat)
            // handle(PrintStat) 
            // handle(IfStat)
            // handle(WhileStat)
            // handle(ReturnStat)
            
            // handle(TernaryExp)
            // handle(OrExp)
            // handle(AndExp)
            // handle(BitOrExp)
            // handle(BitAndExp)
            // handle(BitXorExp)
            // handle(EqExp)
            // handle(NotEqExp)
            // handle(LessExp)
            // handle(GreaterExp)
            // handle(LessEqExp)
            // handle(GreaterEqExp)
            // handle(ShiftLeftExp)
            // handle(ShiftRightExp)
            // handle(AddExp)
            // handle(SubExp)
            // handle(MulExp)
            // handle(DivExp)
            // handle(ModExp)
            // handle(PowExp)
            // handle(NegateExp)
            // handle(NotExp)
            // handle(BitNotExp)
            // handle(LenExp)
            
            // handle(CallExp)
            // handle(ArgList)
            // handle(IndexExp)
            // handle(AccessExp)
            
            // handle(ClockExp)
            // handle(RandomExp)
            
            // handle(FuncLiteral)
                // handle(ParamDef)
                // handle(NumLiteral)
                // handle(StringLiteral)
                // handle(ArrayLiteral)
                // handle(ObjectLiteral)
                // handle(Identifier)

            handle(StatList) {
                for (auto& c: node.children) {
                    compile(c, program);
                    free_all_registers();
                }
                return 0xff;
            }
            handle(VarDeclStat) {
                auto reg = compile(node.children[1], program);
                bind_register(node.children[0].data_s, reg);
                return reg;
            }
            handle(Identifier) { // read variable - returns the register used
                return bindings[node.data_s];
            }
            handle(NumLiteral) { // 1 out via LOAD_CONST
                auto n = node.data_d;
                auto out = allocate_register();
                auto in1 = (uint8_t)program.storage.size();
                program.storage.emplace_back(value_from_number(n));
                emit(LOAD_CONST, out, in1, 0);
                return out;
            }
            handle(FuncLiteral) {
                compiler->add_function(program, &node);
                auto out = allocate_register();
                emit(LOAD_CONST, out, compiler->funcs.back().storage_index, 0);
                return out;
            }
            handle(CallExp) { // 1 out if 
                auto func_reg = compile(node.children[0], program); // LHS evaluates to function
                auto nargs = node.children[0].children.size();
                auto end_reg = get_end_register();
                nargs = 0;// TODO: compile arguments into registers starting from end_reg+2
                emit(CALL, func_reg, nargs, end_reg);
                free_register(func_reg);
                return 0xff;
            }
            handle(ReturnStat) {
                return 0xff;
            }
            handle(AddExp) { // 1 out
                // TODO: experiment with the following:
                //      i1 = compile_node(...)
                //      free_register(i1)
                //      i2 = compile_node(...)
                //      free_register(i2)
                //      out = allocate_register()
                //      ...
                // allows to reuse one of the registers (if not bound)
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(ADD, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(MulExp) { // 1 out
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(MUL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(DivExp) { // 1 out
                auto in1 = compile(node.children[0], program);
                auto in2 = compile(node.children[1], program);
                auto out = allocate_register();
                emit(DIV, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(PrintStat) { // no output
                auto in1 = compile(node.children[0], program);
                emit(PRINT, in1, 0, 0);
                free_register(in1);
                return 0xff;
            }

            default: { throw runtime_error("unknown ast; "); } break;
        }

        throw runtime_error("forgot to return a register");
        return 0;
    }

    #undef handle
    #undef emit
};

void Compiler2::compile(const AstNode& module, Program& program) {
    if (module.type != AstType::StatList) {
        throw runtime_error("expected statlist");
    }
    auto node = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef, {}), module);
    add_function(program, &node);

    // compile all function literals and append to main bytecode
    // TODO: interesting, can this be parallelized?
    for (auto f = funcs.begin(); f != funcs.end(); f++) {
        // can't use range-based for because compiler.functions size changes
        program.storage[f->storage_index] = value_from_function(program.instructions.size());
        f->compile(program);
    }
}
void Compiler2::add_function(Program& program, const AstNode* node) {
    auto storage_index = program.storage.size();
    program.storage.emplace_back(value_from_function(0));
    funcs.emplace_back(FunctionContext { this, node, (uint8_t)storage_index });
}


struct Interpreter2 {
    // Lua only allows 256 registers, we should do the same

    #define handle(opcode) break; case Opcode::opcode:
    void execute(const Program& program) {

        #define REGISTER(n) _stack[_stackbase+n]
        auto _stack = array<Value, 4096> {};
        auto _stackbase = 0u;
        auto _pc = 0u;
        auto _pe = program.instructions.size();

        REGISTER(0) = value_from_function(program.instructions.size()); // pseudo function that jumps to end
        REGISTER(1) = value_from_integer(0); // reset stack base to 0
        _stackbase = 2;

        while (_pc < _pe) {
            auto i = program.instructions[_pc];
            switch (i.opcode) {
                case Opcode::UNKNOWN: break;
                handle(LOAD_CONST) {
                    REGISTER(i.r0) = program.storage[i.r1];
                }
                handle(ADD) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) + value_to_number(REGISTER(i.r2)));
                }
                handle(MUL) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) * value_to_number(REGISTER(i.r2)));
                }
                handle(DIV) {
                    REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) / value_to_number(REGISTER(i.r2)));
                }

                handle(CALL) {
                    auto func_start = value_to_function(REGISTER(i.r0));
                    auto nargs = i.r1;
                    auto end_reg = i.r2;
                    
                    REGISTER(end_reg) = value_from_function(_pc); // push return addr
                    REGISTER(end_reg+1) = value_from_integer(_stackbase); // push return frameptr
                    _pc = func_start - 1; // jump to addr
                    _stackbase = _stackbase + end_reg + 2; // new stack frame
                    
                }
                handle(RET) {
                    _pc = value_to_function(REGISTER(-2));
                    _stackbase = value_to_integer(REGISTER(-1));
                }
                handle(PRINT) {
                    log<false,false>(REGISTER(i.r0));
                }
                break; default: throw runtime_error("unknown instruction: " + to_string(i.opcode));
            }
            _pc++;
        }
    }
};