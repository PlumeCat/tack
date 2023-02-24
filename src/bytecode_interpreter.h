#pragma once

#include <cstring>
#include <list>


// Each instruction has up to 3 arguments
// CONDJUMP r0 i0 i1 
// - if r0 is true, jump to i0, otherwise jump to i1
// TODO: condjump i0 and i1 should be more than 16bit
// can mitigate by making it a relative jump (?) but still problematic
#include <jlib/hash_map.h>

#define opcodes() \
    opcode(UNKNOWN)\
    /* binary operations:*/\
    opcode(EQUAL) opcode(NEQUAL) opcode(GREATER) opcode(LESS) opcode(GREATEREQ) opcode(LESSEQ)\
    opcode(ADD) opcode(SUB) opcode(DIV) opcode(MUL) opcode(MOD) opcode(POW)\
    opcode(SHL) opcode(SHR) opcode(BITAND) opcode(BITOR) opcode(BITXOR) opcode(AND) opcode(OR)\
    /* unary operation: apply inplace for value at top of stack*/\
    opcode(NEGATE) opcode(NOT) opcode(BITNOT)\
    /* stack and variable operations */\
    opcode(LOAD) opcode(STORE) opcode(LOADCONST) opcode(GROW) \
    /* Relative jump using encoded offset */\
    opcode(JUMPF) opcode(JUMPB)\
    /* Relative jump using encoded offset IFF stack[n--] == 0 (but pop the top regardless) */\
    opcode(CONDJUMP)\
    opcode(PRECALL) opcode(CALL) opcode(RET)/**/\
    opcode(PRINT)/**/\
    

#define opcode(x) x,
enum class Opcode : uint8_t { opcodes() };
#undef opcode
#define opcode(x) { Opcode::x, #x },
static hash_map<Opcode, string> opcode_to_string = { opcodes() };
#undef opcode

uint64_t encode_op(Opcode opcode, uint64_t i1 = 0, uint64_t i2 = 0, uint64_t i3 = 0) {
    if (i1 > UINT16_MAX || i2 > UINT16_MAX || i3 > UINT16_MAX) {
        throw runtime_error("Out of range instruction");
    }
    return (
        uint64_t(opcode) << 48 |
        uint64_t(i1) << 32 |
        uint64_t(i2) << 16 |
        uint64_t(i3)
    );
};

struct Program {
    vector<uint64_t> instructions;
    vector<double> storage; // program constant storage goes at the bottom of the stack for now

    string to_string() {
        auto s = stringstream {};

        s << "Program:\n";
        s << "   Bytecode:\n";
        auto i = 0;
        for (auto bc : instructions) {
            auto opcode = uint16_t(bc >> 48);
            auto r1 = uint16_t(bc >> 32);
            auto r2 = uint16_t(bc >> 16);
            auto r3 = uint16_t(bc);
            s << "        " << i << ": " << opcode_to_string[(Opcode)opcode] << ", " << r1 << '\n'; //", " << r2 << ", " << r3 << '\n';
            i++;
        }

        s << "    Storage:\n";
        i = 0;
        for (auto& x : storage) {
            s << "        " << i << ": " << x << '\n';
            i++;
        }

        return s.str();
    }
};

struct Compiler {
    struct VariableContext {
        string name;
        uint32_t stackpos;
        bool is_captured = false; // if captured by a lower scope, allocate on heap
    };
    struct ScopeContext {
        hash_map<string, VariableContext> variables;
    };
    struct FunctionContext {
        const AstNode* func_node; // before compilation
        vector<ScopeContext> scopes;
        FunctionContext* parent_context; // so we can capture variables from lexical parent scopes
        uint32_t storage_location;
        uint32_t variable_count;

        // number string object array function
        
        // variable -> stack index relative to current stack frame
        int lookup_variable(const string& identifier) {
            // TODO: search in higher scopes starting from parent scope
            // if the variable is found in a parent scope, mark as is_captured
            // captured variables must be auto boxed and garbage collected/reference counted
            // (we can also do escape analysis, eg if a reference to child function does not escape a parent function,
            // then it can refer to captured variables via the stack)
            for (auto s = scopes.rbegin(); s != scopes.rend(); s++) {
                if (auto iter = (*s).variables.find(identifier); iter != (*s).variables.end()) {
                    log(identifier, " -> ", iter->second.stackpos);
                    return iter->second.stackpos;
                }
            }
            return -1;
        }
        
        // set stack position for new variable
        // don't bind if already exists in current scope
        int bind_variable(const string& identifier) {
            if (scopes.back().variables.find(identifier) != scopes.back().variables.end()) {
                return -1;
            }
            auto stackpos = variable_count;
            scopes.back().variables[identifier] = VariableContext { identifier, stackpos, false };
            variable_count += 1;
            return stackpos;
        }

        void push_scope() {
            scopes.push_back({});
        }
        void pop_scope() {
            scopes.pop_back();
        }
    };
    
    list<FunctionContext> funcs; // list not vector - need to push while iterating and not invalidate reference. queue would be even better


    void add_function(const AstNode* node, uint32_t storage_location = UINT32_MAX, FunctionContext* parent_context = nullptr) {
        funcs.emplace_back(FunctionContext { node, {}, parent_context, storage_location });
    }
    
    void compile(const AstNode& module, Program& program) {
        // treat the top level of the program as a function body
        // RET will be emitted for it - don't need a special EXIT instruction
        // when the program is entered, the 0th stack frame has the last instruction+1
        // as the return address, so the interpreter loop can terminate
        // (see 'execute')
        auto node = AstNode(
            AstType::FuncLiteral,
            AstNode(AstType::ParamDef, {}),
            module // parsing ensures that module is always a stat-list
        );

        // program.storage.emplace_back(0); // dummy address of top level body
        // functions.emplace_back(pair { &node, program.storage.size() });
        add_function(&node);

        // compile all function literals and append to main bytecode
        // TODO: interesting, can this be parallelized?
        for (auto f = funcs.begin(); f != funcs.end(); f++) { // can't use range-based for because compiler.functions size changes
            auto& func = *f;
            if (func.storage_location != UINT32_MAX) {
                program.storage[func.storage_location] = program.instructions.size();
            }
            compile_function(func, program);
        }
    }

    #define encode(op, ...) encode_op(Opcode::op, __VA_ARGS__)
    #define emit(op, ...) program.instructions.push_back(encode(op, __VA_ARGS__));
    #define rewrite(instr, op, ...) program.instructions[instr] = encode(op, __VA_ARGS__);
    #define handle(type)                break; case AstType::type:
    #define handle_binexp(type, opcode) handle(type) { child(0); child(1); emit(opcode, 0); }
    #define child(n) compile_node(context, node.children[n], program);
    #define label(name) auto name = program.instructions.size();
    
    void compile_function(FunctionContext& context, Program& program) {
        auto num_args = context.func_node->children[0].children.size();
        
        // grow stack to make space for variables (space required will be known later)
        label(grow); emit(GROW, 0);
        
        // handle arguments; bind as variables
        context.push_scope();
        for(const auto& arg: context.func_node->children[0].children) {
            // bind arg as variable
            // it's at the top of the stack so it's mutable
            auto stackpos = context.bind_variable(arg.data_s);
            if (stackpos == -1) {
                throw runtime_error("Compile error: duplicate argument: " + arg.data_s);
            }
        }
        // compile body
        compile_node(context, context.func_node->children[1], program);
        context.pop_scope();
        
        // return
        emit(RET, 0);
        
        // space for variables
        auto num_variables = context.variable_count;
        auto extra_stack = num_variables - num_args; // extra space for arguments not needed, CALL will handle, we just need to allow space for variable bindings
        rewrite(grow, GROW, max(extra_stack, (size_t)0));
    }

    void compile_node(FunctionContext& context, const AstNode& node, Program& program) {
        switch (node.type) {
            case AstType::Unknown: return;
            
            handle(StatList) {
                context.push_scope();
                for (auto i = 0; i < node.children.size(); i++) {
                    child(i); // TODO: discard expression statements from stack (ideally they aren't permitted)
                }
                context.pop_scope();
            }
            handle(VarDeclStat) {
                child(1); // push result of expression
                
                // make sure not already declared
                // TODO: allow shadowing in lower scopes?
                auto stackpos = context.bind_variable(node.children[0].data_s);
                if (stackpos == -1) {
                    throw runtime_error("Compile error: variable already exists: " + node.children[0].data_s);
                }
                
                emit(STORE, stackpos); // put the value at the top of the stack into stackpos
            }
            handle(AssignStat) {
                child(1);
                
                // make sure variable exists
                auto stackpos = context.lookup_variable(node.children[0].data_s);
                if (stackpos == -1) {
                    throw runtime_error("Compile error: variable not found: " + node.children[0].data_s);
                }
                
                emit(STORE, stackpos); // value at top of stack into stackpos
            }
            handle(PrintStat) {
                child(0);
                emit(PRINT, 0);
            }
            handle(IfStat) {
                child(0); // evaluate the expression
                
                label(condjump);emit(CONDJUMP, 0, 0, 0); // PLACEHOLDER
                                child(1); // compile the if body
                label(endif);   emit(JUMPF, 0, 0, 0); // PLACEHODLER TODO: not needed if there's no else-block

                label(startelse);if (node.children.size() == 3)
                                child(2); // compile the else body
                label(endelse);
                
                // TODO: make more efficient
                // can be more efficient for the case when there's no else (probably)
                rewrite(condjump, CONDJUMP, startelse - condjump);
                rewrite(endif, JUMPF, endelse - endif);
            }
            handle(WhileStat) {
                label(cond);    child(0); // evaluate expression
                
                label(condjump);emit(CONDJUMP, 0, 0, 0); // placeholder
                                child(1); // compile the body
                label(jumpback);emit(JUMPB, jumpback - cond); // jump to top of loop
                label(ewhile);

                rewrite(condjump, CONDJUMP, ewhile - condjump);
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

            handle(NegateExp) { child(0); emit(NEGATE, 0); }
            handle(NotExp)    { child(0); emit(NOT, 0); }
            handle(BitNotExp) { child(0); emit(BITNOT, 0); }

            handle(ReturnStat) {
                // TODO: handle return value
                auto nret = node.children.size(); // should be 0 or 1
                if (nret) {
                    child(0); // push return value
                }
                emit(RET, nret);
            }
            handle(CallExp) {
                // TODO: optimize direct calls vs indirect calls with some kind of symbol table?
                // RHS of call expression is an ArgList; evaluate all arguments
                auto nargs = node.children[1].children.size();
                emit(PRECALL, 0);
                child(1); // ArgList                
                child(0); // LHS of the call exp evaluates to storage location of a function address
                emit(CALL, nargs); // call function at top of stack
            }
            handle(ArgList) {
                for (auto i = 0; i < node.children.size(); i++) {
                    child(i);
                }
            }
            handle(IndexExp) {}
            handle(AccessExp) {}
            handle(FuncLiteral) {
                // put a placeholder value in storage at first. when the program is linked, the placeholder is
                // overwritten with the real start address
                auto storage = program.storage.size();
                program.storage.push_back(0); // placeholder value
                // funcs.emplace_back(pair { &node, func_storage }); // compile function
                add_function(&node, storage, &context);

                // load the function start address
                emit(LOADCONST, storage);
            }
            handle(ParamDef) {}
            handle(NumLiteral) {
                // TODO: don't store the same constant more than once
                program.storage.push_back(node.data_d);
                emit(LOADCONST, program.storage.size() - 1);
            }
            handle(StringLiteral) {}
            handle(ArrayLiteral) {}
            handle(ObjectLiteral) {}
            handle(Identifier) {
                // reading a variable (writing is not encountered by recursive compile)
                // so the variable exists and points to a stackpos
                auto stackpos = context.lookup_variable(node.data_s);
                if (stackpos == -1) {
                    throw runtime_error("Compile error: variable not found: " + node.data_s);
                }
                emit(LOAD, stackpos);
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
    
    void execute(Program& program) {
        // TODO: optimization, computed goto, direct threading, ...
        
        // HACK: sort of.
        // the top level block of the program counts as the 0th stack frame
        // so when it RETs, it needs to jump to the final instruction so the interpreter loop can terminate
        auto instruction_pointer = 0;
        const auto STACK_SIZE = 1024;
        double stack[STACK_SIZE]; memset((void*)stack, 0xffffffff, sizeof(double) * STACK_SIZE);
        auto stackptr = 0;
        auto stackbase = 0;

        auto stack_push = [&](double d) {
            stack[stackptr++] = d;
        };
        auto stack_pop = [&]() -> double {
            return stack[--stackptr];
        };

        stack_push(program.instructions.size()); // base return address is end of code
        stack_push(0); // base frame ptr
        stackbase = 2;

        while (instruction_pointer < program.instructions.size()) {
            // decode
            auto instruction = program.instructions[instruction_pointer];
            auto opcode = uint16_t((instruction >> 48) & 0xffff);
            auto r1 = uint16_t((instruction >> 32) & 0xffff);
            auto r2 = uint16_t((instruction >> 16) & 0xffff);
            auto r3 = uint16_t((instruction) & 0xffff);

            #define handle(c)           break; case (uint16_t)Opcode::c:
            #define handle_binop(c,op) handle(c) { stack[stackptr-2] = stack[stackptr-2] op stack[stackptr-1]; stackptr--; }

            // TODO: clean up interaction between CALL and GROW
            // CALL is muching the stack pointer / stack size, then GROW mucks it again,
            // then will need more mucking to handle args and return
            // maybe CALL / RET need to do more work, or store function metadata somewhere?
                // could put function fixed stack growth into storage after the start address?
            // execute
            switch (opcode) {
                case (uint16_t)Opcode::UNKNOWN:
                // binop
                handle_binop(ADD, +)
                handle_binop(SUB, -)
                handle_binop(MUL, *)
                handle_binop(DIV, /)
                handle_binop(EQUAL,    ==)
                handle_binop(NEQUAL,   !=)
                handle_binop(LESS,     <)
                handle_binop(GREATER,  >)
                handle_binop(LESSEQ,   <=)
                handle_binop(GREATEREQ,>=)
                handle(MOD)             { stack[stackptr-2] = (double)(((uint64_t)stack[stackptr-2]) % ((uint64_t)stack[stackptr-1])); stackptr--; }
                // unop
                handle(NEGATE)          { stack[stackptr-1] = -stack[stackptr-1]; }

                handle(GROW)            { stackptr += r1; }
                handle(LOAD)            { stack_push(stack[stackbase + r1]); }
                handle(LOADCONST)       { stack_push(program.storage[r1]); }
                handle(STORE)           { stack[stackbase+r1] = stack_pop(); }
                handle(PRINT)           { log<true, false>("print:", stack_pop()); }
                
                handle(JUMPF)           { instruction_pointer += r1 - 1; }
                handle(JUMPB)           { instruction_pointer -= (r1 + 1); }
                handle(CONDJUMP)        { if (!stack_pop()) { instruction_pointer += r1 - 1; }}
                
                handle(PRECALL) {
                    stack_push(0); // placeholder for return address
                    stack_push(stackbase); // return frame pointer
                }
                handle(CALL) {
                    auto call_addr = stack_pop();
                    stackbase = stackptr - r1;// arguments were already pushed
                    stack[stackbase - 2] = instruction_pointer;// rewrite the return address lower down the stack
                    instruction_pointer = call_addr - 1; // -1 because we have instruction_pointer++ later; (TODO: optimize out?)
                }
                handle(RET) {
                    auto retval = 0;
                    if (r1) {
                        retval = stack_pop();
                    }
                    stackptr = stackbase;                    
                    stackbase = stack_pop();
                    instruction_pointer = stack_pop();
                    if (r1) {
                        stack_push(retval);
                    }
                }
                break; default: throw runtime_error("unknown instruction "s + opcode_to_string[(Opcode)opcode]);
            }

            instruction_pointer += 1;

            #undef handle
            #undef handle_binop
        }
    }
};

