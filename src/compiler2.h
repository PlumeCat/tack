#pragma once

#include <list>
#include  <jlib/swiss_vector.h>
#include "program.h"
#include "value.h"

// emit an instruction with 3 8 bit operands
#define emit(op, r0, r1, r2)            instructions.emplace_back(Opcode::op, r0, r1, r2)

// emit an instruction with 1 u8 operand r and 1 u16 operand u
#define emit_u(op, r, u)                auto ins = Instruction{}; ins.opcode = Opcode::op; ins.r0 = r; ins.u1 = u; instructions.emplace_back(ins);

// rewrite an instruction with 3 8-bit operands
#define rewrite(pos, type, r0, r1, r2)  instructions[pos] = { Opcode::type, r0, r1, r2 };
#define label(name)                     auto name = instructions.size();
#define should_allocate(n)
#define handle(x)                       break; case AstType::x:


bool is_integer(double d) {
    return trunc(d) == d;
}

bool is_small_integer(double d) {
    return is_integer(d) && 
        d <= std::numeric_limits<int16_t>::max() &&
        d >= std::numeric_limits<int16_t>::min();
}

enum RegisterState {
    FREE = 0,
    BUSY = 1,
    BOUND = 2
};

using StorageIndex = uint16_t;
struct ScopeContext;
struct Program;
//struct Compiler2 {
//    list<FunctionContext> funcs; // need to grow without invalidating pointers
//    
//    // returns the storage index of the start address
//    FunctionContext& add_function(Program& program, ScopeContext* parent_scope, const AstNode* node);
//    
//    void compile(const AstNode& module, Program& program);
//};

static const uint32_t MAX_REGISTERS = 256;
static const uint32_t STACKFRAME_OVERHEAD = 4;

struct VariableContext {
    uint8_t reg;
    bool is_const = false;
};
struct ScopeContext {
    Program* function;
    ScopeContext* parent_scope = nullptr; // establishes a tree structure over FunctionContext::scopes
    bool is_top_level_scope = false;
    hash_map<string, VariableContext> bindings = hash_map<string, VariableContext>(1, 1); // variables

    // needs_capture = the lookup is happening in an enclosed function
    VariableContext* lookup(const string& name);
};
struct Program {
    // Compiler2* compiler;
    const AstNode* node;
    // ScopeContext* parent_scope = nullptr; // enclosing scope - might belong to parent function
    // StorageIndex storage_index;
    string name;

    array<RegisterState, MAX_REGISTERS> registers;
    list<ScopeContext> scopes;
    vector<CaptureInfo> captures;

    // Program& program;
    vector<Instruction> instructions;
    vector<Value> storage; // program constant storage goes at the bottom of the stack for now
    list<string> strings; // string constants and literals storage - includes identifiers for objects
    list<Program> functions;

    string str(const string& prefix = ""s) {
        auto s = stringstream{};
        s << "function: " << prefix + name << endl;
        auto i = 0;
        for (auto bc : instructions) {
            s << "    " << i << ": " << ::to_string(bc.opcode) << ' ' << (uint32_t)bc.r0 << ' ' << (uint32_t)bc.r1 << ' ' << (uint32_t)bc.r2 << '\n';
            i++;
        }

        s << "  storage:\n";
        i = 0;
        for (auto& x : storage) {
            // s << "    " << i << ": " << x << '\n';
            i++;
        }

        s << "  strings:\n";
        i = 0;
        for (auto& _s: strings) {
            // s << "    " << i << ": " << _s << '\n';
            i++;
        }

        for (auto& f : functions) {
            s << f.str(prefix + name + "::");
        }
        return s.str();
    }

    //// lookup a local variable
    //VariableContext* lookup_local(const string& name) {
    //    // TODO: handle globals as a special case
    //    // may want more than 256 globals especially for functions
    //    for (auto s = current_scope; !s->is_top_level_scope && s->parent_scope; s = s->parent_scope) {
    //        if (auto iter = s->bindings.find(name); iter != s->bindings.end()) {
    //            return &iter->second;
    //        }
    //    }
    //    return nullptr;
    //}

    //// lookup a local variable in the enclosing function (or enclosing enclosing function, etc)
    //// may emit code to read captured value into register
    //VariableContext* lookup_parent(const string& name) {
    //    auto s = parent_scope; // parent scope containing function
    //    while (true) {
    //        if (auto iter = s->bindings.find(name); iter != s->bindings.end()) {
    //            // if there is a capture, we make a new local variable for the inner function
    //            // at ALLOC_FUNC, the local variable from the outer function is put into a box
    //            // the register containing the variable now contains the box
    //            // the owning function can continue to use it transparently from there (with the box penalty)

    //            // needs optimization because now registers need to check if boxed all the time
    //            // we could mark registers as containing boxes, and emit READ_BOX/WRITE_BOX as necessary
    //            // however still going to need to rewrite parent function code until we have proper recursive single pass compile

    //            auto reg = allocate_register();
    //            captures.emplace_back(CaptureInfo { .source_register = iter->second.reg, .dest_register = reg });
    //            emit(READ_CAPTURE, reg, captures.size() - 1, 0);
    //            return bind_register(name, reg, iter->second.is_const);
    //        }
    //        if (s->is_top_level_scope && s->parent_scope) {
    //            // only allow 1 level of capture for the time being

    //            break;
    //        }
    //        s = s->parent_scope;
    //    }

    //    return nullptr;
    //}

    VariableContext* lookup(const string& name) {
        return scopes.back().lookup(name);
    }


    // get a free register
    uint8_t allocate_register() {
        // TODO: lifetime analysis for variables
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
            if (registers[i-1] != FREE) {
                return i;
            }
        }
        return 0;
    }

    // set a register as bound by a variable
    VariableContext* bind_register(const string& binding, uint8_t reg, bool is_const = false) {
        registers[reg] = BOUND;
        return &scopes.back().bindings.insert(binding, {reg, is_const})->second;
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

    void push_scope(bool is_top_level = false, ScopeContext* parent_scope = nullptr) {
        scopes.emplace_back(ScopeContext { this, parent_scope, is_top_level});
    }
    void pop_scope() {
        // unbind all bound registers
        for (auto& b: scopes.back().bindings) {
            //registers[b.second.reg] = FREE; // TODO: un-disable once capture is working, and see if still works
        }
        scopes.pop_back();
    }
    

    uint16_t store_number(double d) {
        storage.emplace_back(value_from_number(d));
        return storage.size() - 1;
    }

    uint16_t store_string(const string& data) {
        strings.emplace_back(data);
        storage.emplace_back(value_from_string(
            &strings.back()
        ));
        return storage.size() - 1;
    }
    uint16_t store_function() {
        functions.emplace_back();
        storage.emplace_back(value_from_function(&functions.back()));
        return storage.size() - 1;
    }
   

    void compile(ScopeContext* parent_scope) {
        // compile function literal
        // TODO: emit bindings for the N arguments
        push_scope(true, parent_scope);
        auto nargs = node->children[0].children.size(); // ParamDef
        for (auto i = 0; i < nargs; i++) {
            auto& param = node->children[0].children[i]; // Identifier
            bind_register(param.data_s, i, false); // TODO: could choose to make arguments const?
        }
        auto reg = compile(node->children[1]);
        emit(RET, 0, 0, 0);
        pop_scope();
    }
    uint8_t compile(const AstNode& node, uint8_t target_register = 0xff) {
        switch (node.type) {
            case AstType::Unknown: {} break;
            handle(StatList) { should_allocate(0);
                push_scope(false, &scopes.back());
                for (auto& c: node.children) {
                    compile(c);
                    free_all_registers();
                }
                pop_scope();
                return 0xff;
            }
            handle(IfStat) {
                auto has_else = node.children.size() == 3;
                auto cond_reg = compile(node.children[0]); // evaluate the expression
                label(condjump);
                emit(CONDJUMP, cond_reg, 0, 0); // PLACEHOLDER
                free_register(cond_reg); // can be reused
                compile(node.children[1]); // compile the if body
                label(endif);
                
                if (has_else) {
                    emit(JUMPF, 0, 0, 0); // PLACEHODLER
                    label(startelse);
                    compile(node.children[2]);
                    label(endelse);
                    if (endelse - endif > 0xff) {
                        throw runtime_error("jump too much");
                    }
                    rewrite(endif, JUMPF, uint8_t(endelse - endif), 0, 0);
                    if (startelse - condjump > 0xff) {
                        throw runtime_error("jump too much");
                    }
                    rewrite(condjump, CONDJUMP, cond_reg, uint8_t(startelse - condjump), 0);
                } else {
                    if (endif - condjump > 0xff) {
                        throw runtime_error("jump too much");
                    }
                    rewrite(condjump, CONDJUMP, cond_reg, uint8_t(endif - condjump), 0);
                }
                return 0xff;
            }
            handle(WhileStat) {
                label(condeval);
                auto cond_reg = compile(node.children[0]);
                label(condjump);
                emit(CONDJUMP, cond_reg, 0, 0);
                free_register(cond_reg);
                compile(node.children[1]);
                label(jumpback);
                if (jumpback - condeval > 0xff) {
                    throw runtime_error("jump too much");
                }
                emit(JUMPB, uint8_t(jumpback - condeval), 0, 0);
                label(endwhile);
                if (endwhile - condjump > 0xff) {
                    throw runtime_error("jump too much");
                }
                rewrite(condjump, CONDJUMP, cond_reg, uint8_t(endwhile - condjump), 0);
                return 0xff;
            }
            handle(VarDeclStat) { should_allocate(1);
                auto reg = compile(node.children[1]);
                bind_register(node.children[0].data_s, reg);
                return reg;
            }
            handle(ConstDeclStat) { should_allocate(1);
                auto reg = compile(node.children[1]);
                bind_register(node.children[0].data_s, reg, true);
                return reg;
            }
            handle(AssignStat) { should_allocate(0);
                auto source_reg = compile(node.children[1]);
                auto& lhs = node.children[0];
                if (lhs.type == AstType::Identifier) {
                    if (auto var = lookup(node.children[0].data_s)) {
                        if (var->is_const) {
                            throw runtime_error("can't reassign const variable");
                        }
                        // TODO: try and elide this MOVE
                        // if the moved-from register is "busy" but not "bound", then the value is not used again
                        // only need MOVE if assigning directly from another variable
                        // in which case MOVE is more like COPY
                        emit(MOVE, var->reg, source_reg, 0);
                    } else {
                        throw runtime_error("can't find variable: "s + node.children[0].data_s);
                    }
                } else if (lhs.type == AstType::IndexExp) {
                    auto array_reg = compile(lhs.children[0]);
                    auto index_reg = compile(lhs.children[1]);
                    emit(STORE_ARRAY, source_reg, array_reg, index_reg);
                    free_register(index_reg);
                    free_register(array_reg);
                } else if (lhs.type == AstType::AccessExp) {
                    auto obj_reg = compile(lhs.children[0]);
                    // save identifier as string and load it
                    auto key_reg = allocate_register();
                    auto index = store_string(lhs.children[1].data_s);
                    emit_u(LOAD_CONST, key_reg, index);                    
                    emit(STORE_OBJECT, source_reg, obj_reg, key_reg);
                    free_register(obj_reg);
                    free_register(key_reg);
                }
                free_register(source_reg);
                return source_reg; // unused
            }
            handle(Identifier) { should_allocate(0);
                if (auto v = lookup(node.data_s)) {
                    return v->reg;
                } else {
                    throw runtime_error("can't find variable: "s + node.data_s);
                }
                return 0xff;
            }
            handle(NumLiteral) { should_allocate(1);
                auto n = node.data_d;
                auto out = allocate_register();
                auto index = store_number(node.data_d);
                emit_u(LOAD_CONST, out, index);
                return out;
            }
            handle(StringLiteral) { should_allocate(1);
                auto out = allocate_register();
                auto index = store_string(node.data_s);
                emit_u(LOAD_CONST, out, index);
                return out;
            }
            handle(FuncLiteral) { should_allocate(1);
                // note this may create some captures and emit READ_CAPTURE
                // auto& func = compiler->add_function(program, current_scope, &node);
                auto index = store_function();
                auto func = value_to_function(storage[index]);
                func->node = &node;
                func->name = (node.children.size() == 3) ? node.children[2].data_s : "(anonymous)";
                func->compile(&scopes.back());
                
                // load an initial closure
                auto out = allocate_register();
                emit_u(ALLOC_FUNC, out, index);

                // add a const binding if it's a named function
                if (node.children.size() == 3) {
                    bind_register(node.children[2].data_s, out, true);
                }

                return out;
            }
            handle(CallExp) { should_allocate(1);
                auto nargs = node.children[1].children.size();

                // compile the arguments first and remember which registers they are in
                auto arg_regs = vector<uint8_t>{};
                for (auto i = 0; i < nargs; i++) {
                    arg_regs.emplace_back(compile(node.children[1].children[i]));
                }
                auto func_reg = compile(node.children[0]); // LHS evaluates to function

                // copy arguments to top of stack in sequence
                auto end_reg = get_end_register();
                for (auto i = 0; i < nargs; i++) {
                    emit(MOVE, end_reg + i + STACKFRAME_OVERHEAD, arg_regs[i], 0);
                }
                
                emit(CALL, func_reg, nargs, end_reg);
                
                // return value goes in end-reg so mark it as used
                registers[end_reg] = BUSY;
                return end_reg; // return value copied to end register
            }
            handle(ClockExp) {
                auto reg = allocate_register();
                emit(CLOCK, reg, 0, 0);
                return reg;
            }
            handle(RandomExp) {
                auto reg = allocate_register();
                emit(RANDOM, reg, 0, 0);
                return reg;
            }
            handle(PrintStat) { // no output
                auto in1 = compile(node.children[0]);
                emit(PRINT, in1, 0, 0);
                free_register(in1);
                return 0xff;
            }
            handle(ReturnStat) {
                if (node.children.size()) {
                    auto return_register = compile(node.children[0]);
                    if (return_register == 0xff) {
                        throw runtime_error("return value incorrect register");
                        // emit(RET, 0, 0, 0);
                    }
                    emit(RET, 1, return_register, 0);
                } else {
                    emit(RET, 0, 0, 0);
                }
                return 0xff; // it's irrelevant
            }
            handle(LenExp) {
                auto in = compile(node.children[0]);
                auto out = allocate_register();
                emit(LEN, out, in, 0);
                free_register(in);
                return out;
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
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(ADD, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(SubExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(SUB, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(LessExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(LESS, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(LessEqExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(LESSEQ, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(EqExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(EQUAL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(NotEqExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(NEQUAL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(GreaterExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(GREATER, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(GreaterEqExp) {
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(GREATEREQ, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(MulExp) { // 1 out
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(MUL, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(DivExp) { // 1 out
                auto in1 = compile(node.children[0]);
                auto in2 = compile(node.children[1]);
                auto out = allocate_register();
                emit(DIV, out, in1, in2);
                free_register(in1);
                free_register(in2);
                return out;
            }
            handle(ArrayLiteral) {
                auto reg = allocate_register();
                // TODO: child elements
                emit(ALLOC_ARRAY, reg, 0, 0);
                return reg;
            }
            handle(ObjectLiteral) {
                auto reg = allocate_register();
                emit(ALLOC_OBJECT, reg, 0, 0);
                return reg;
            }
            handle(ShiftLeftExp) {
                auto arr = compile(node.children[0]);
                auto value = compile(node.children[1]);
                emit(SHL, arr, value, 0);
                return 0xff;
            }
            handle(IndexExp) {
                auto arr = compile(node.children[0]);
                auto ind = compile(node.children[1]);
                auto out = allocate_register();
                emit(LOAD_ARRAY, out, arr, ind);
                free_register(arr);
                free_register(ind);
                return out;
            }
            handle(AccessExp) {
                // TODO: object read
                auto obj = compile(node.children[0]);
                
                // save identifier as string and load it
                auto key = allocate_register();
                auto index = store_string(node.children[1].data_s);
                emit_u(LOAD_CONST, key, index);
                
                // load from object
                auto out = allocate_register();
                emit(LOAD_OBJECT, out, obj, key);
                free_register(obj);
                free_register(key);
                return out;
            }

            default: { throw runtime_error("unknown ast: " + to_string(node.type)); } break;
        }

        throw runtime_error("forgot to return a register");
        return 0;
    }

};

VariableContext* ScopeContext::lookup(const string& name) {
    if (auto iter = bindings.find(name); iter != bindings.end()) {
        return &iter->second;
    }

    if (parent_scope) {
        // not found by parent scope
        if (auto var = parent_scope->lookup(name)) {
            if (is_top_level_scope) { // handle capture scenario
                // create a mirroring local variable
                auto mirror_reg = function->allocate_register();
                auto mirror = function->bind_register(name, mirror_reg, var->is_const);

                // record necessary capture into mirror variable
                function->captures.emplace_back(CaptureInfo { .source_register = var->reg, .dest_register = mirror_reg });

                // emit code to read into mirror variable when the time comes
                // auto& program = *function;
                // TODO: terrible macro hack
                function->emit(READ_CAPTURE, mirror_reg, function->captures.size() - 1, 0);

                // return the MIRROR variable not the original!
                return mirror;
            } else {
                // normal scenario
                return var;
            }
        }
    }

    return nullptr;
}

#undef handle
#undef should_allocate
#undef label
#undef rewrite
#undef emit_u
#undef emit

//void Compiler2::compile(const AstNode& module, Program& program) {
//    if (module.type != AstType::StatList) {
//        throw runtime_error("expected statlist");
//    }
//
//    auto node = AstNode(AstType::FuncLiteral, AstNode(AstType::ParamDef, {}), module);
//    add_function(program, nullptr, &node);
//
//    // compile all function literals and append to main bytecode
//    // TODO: interesting, can this be parallelized?
//    for (auto f = funcs.begin(); f != funcs.end(); f++) {
//        // can't use range-based for because compiler.functions size changes
//        auto start_addr = program.instructions.size();
//        f->compile();
//        program.functions.emplace_back(FunctionType { (uint32_t)start_addr, f->captures });
//        program.storage[f->storage_index] = value_from_function(&program.functions.back());
//    }
//
//    // for each function, box captured variables
//}
//FunctionContext& Compiler2::add_function(Program& program, ScopeContext* parent_scope, const AstNode* node) {
//    auto storage_index = program.storage.size();
//    program.storage.emplace_back(value_null());
//    funcs.emplace_back(FunctionContext {
//        .compiler = this,
//        .node = node,
//        .parent_scope = parent_scope,
//        .storage_index = (StorageIndex) storage_index,
//        .program = program
//    });
//    return funcs.back();
//}
//

struct Interpreter2 {
    // Lua only allows 256 registers, we should do the same

    void execute(const Program& program) {
        #define handle(opcode) break; case Opcode::opcode:

        // TODO: likely super inefficient
        #define REGISTER_RAW(n) _stack[_stackbase+n]
        #define REGISTER(n)\
            *(value_is_boxed(REGISTER_RAW(n)) ? value_to_boxed(REGISTER_RAW(n)) : &REGISTER_RAW(n))

        // program counter
        auto _pr = &program; // current program
        auto _pc = 0u;
        auto _pe = program.instructions.size();

        // stack/registers
        auto _stack = array<Value, 4096> {};
        auto _stackbase = STACKFRAME_OVERHEAD;
        _stack.fill(value_null());
        _stack[0]._i = program.instructions.size(); // pseudo return address
        _stack[1]._p = (void*)&program; // pseudo return program
        _stack[2]._i = 0; // reset stack base to 0
        
        // heap
        auto _arrays = list<ArrayType>{};
        auto _objects = list<ObjectType>{};
        auto _closures = list<ClosureType>{};
        auto _boxes = list<Value>{};
        auto error = [&](auto err) { throw runtime_error(err); return value_null(); };

        try {
            while (_pc < _pe) {
                auto i = _pr->instructions[_pc];
                switch (i.opcode) {
                    case Opcode::UNKNOWN: break;
                    handle(LOAD_I) {
                        REGISTER(i.r0) = value_from_integer(i.s1);
                    }
                    handle(LOAD_CONST) {
                        REGISTER(i.r0) = _pr->storage[i.r1];
                    }
                    handle(ADD) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) + value_to_number(REGISTER(i.r2)));
                    }
                    handle(SUB) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) - value_to_number(REGISTER(i.r2)));
                    }
                    handle(MUL) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) * value_to_number(REGISTER(i.r2)));
                    }
                    handle(DIV) {
                        REGISTER(i.r0) = value_from_number(value_to_number(REGISTER(i.r1)) / value_to_number(REGISTER(i.r2)));
                    }
                    // TODO: Bin ops need type checking
                    handle(SHL) {
                        auto* arr = value_to_array(REGISTER(i.r0));
                        arr->emplace_back(REGISTER(i.r1));
                    }
                    handle(LESS) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) <
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(EQUAL) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) ==
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(NEQUAL) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) !=
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(LESSEQ) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) <=
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(GREATER) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) >
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(GREATEREQ) {
                        REGISTER(i.r0) = value_from_boolean(
                            value_to_number(REGISTER(i.r1)) >=
                            value_to_number(REGISTER(i.r2))
                        );
                    }
                    handle(MOVE) { REGISTER(i.r0) = REGISTER(i.r1); }
                    handle(CONDJUMP) {
                        auto val = REGISTER(i.r0);
                        if (!(
                            (value_get_type(val) == Type::Boolean && value_to_boolean(val)) ||
                            (value_get_type(val) == Type::Integer && value_to_integer(val)) ||
                            (value_get_type(val) == Type::Number && value_to_number(val))
                        )) { _pc += i.r1 - 1; }
                    }
                    handle(JUMPF) { _pc += i.r0 - 1; }
                    handle(JUMPB) { _pc -= i.r0 + 1; }
                    handle(LEN) {
                        auto* arr = value_to_array(REGISTER(i.r1));
                        REGISTER(i.r0) = value_from_number(arr->size());
                    }
                    handle(ALLOC_FUNC) {
                        // create closure
                        auto func = value_to_function(_pr->storage[i.u1]);
                        _closures.emplace_back(ClosureType {func, {}});
                        
                        // capture captures
                        // box new captures (TODO: does this work?)
                        auto closure = &_closures.back();
                        for (auto c : func->captures) {
                            auto val = REGISTER_RAW(c.source_register);
                            if (!value_is_boxed(val)) {
                                _boxes.emplace_back(val);
                                auto box = value_from_boxed(&_boxes.back());
                                closure->captures.emplace_back(box);
                                // box existing variable inplace
                                REGISTER_RAW(c.source_register) = box;
                            } else {
                                closure->captures.emplace_back(val);
                            }
                        }

                        // done
                        REGISTER(i.r0) = value_from_closure(closure);
                    }
                    handle(READ_CAPTURE) {
                        REGISTER_RAW(i.r0) = value_to_array(REGISTER_RAW(-1))->at(i.r1);
                    }
                    handle(ALLOC_ARRAY) {
                        _arrays.emplace_back(ArrayType{});
                        REGISTER(i.r0) = value_from_array(&_arrays.back());
                    }
                    handle(ALLOC_OBJECT) {
                        _objects.emplace_back(ObjectType{});
                        REGISTER(i.r0) = value_from_object(&_objects.back());
                    }
                    handle(LOAD_ARRAY) {
                        auto arr_val = REGISTER(i.r1);
                        auto ind_val = REGISTER(i.r2); 
                        auto* arr = value_to_array(arr_val);
                        auto ind_type = value_get_type(ind_val);
                        auto ind = (ind_type == Type::Integer)
                            ? value_to_integer(ind_val) 
                            : (ind_type == Type::Number) 
                                ? (int)value_to_number(ind_val)
                                : error("expected number")._i;

                        if (ind >= arr->size()) {
                            error("outof range index");
                        }
                        REGISTER(i.r0) = (*arr)[ind];
                    }
                    handle(STORE_ARRAY) {
                        auto arr_val = REGISTER(i.r1);
                        if (value_get_type(arr_val) != Type::Array) {
                            error("expected array");
                        }
                        auto* arr = value_to_array(arr_val);
                        
                        auto ind_val = REGISTER(i.r2); 
                        auto ind_type = value_get_type(ind_val);
                        auto ind = (ind_type == Type::Integer)
                            ? value_to_integer(ind_val) 
                            : (ind_type == Type::Number) 
                                ? (int)value_to_number(ind_val)
                                : error("expected number")._i;
                        
                        if (ind > arr->size()) {
                            error("outof range index");
                        }
                        (*arr)[ind] = REGISTER(i.r0);
                    }
                    handle(LOAD_OBJECT) {
                        auto obj_val = REGISTER(i.r1);
                        auto key_val = REGISTER(i.r2);
                        auto* obj = value_to_object(obj_val);
                        if (value_get_type(key_val) != Type::String) {
                            error("expected string key");
                        }
                        auto iter = obj->find(*value_to_string(key_val));
                        if (iter == obj->end()) {
                            error("key not found");
                        }
                        REGISTER(i.r0) = iter->second;
                    }
                    handle(STORE_OBJECT) {
                        auto obj_val = REGISTER(i.r1);
                        auto key_val = REGISTER(i.r2);
                        auto* obj = value_to_object(obj_val);
                        if (value_get_type(key_val) != Type::String) {
                            error("expected string key");
                        }
                        (*obj)[*value_to_string(key_val)] = REGISTER(i.r0);
                    }
                    handle(CALL) {
                        auto closure = value_to_closure(REGISTER(i.r0));
                        auto nargs = i.r1;
                        auto new_base = i.r2 + STACKFRAME_OVERHEAD;
                        REGISTER_RAW(new_base - 4)._i = _pc; // push return addr
                        REGISTER_RAW(new_base - 3)._p = (void*)_pr; // return program
                        REGISTER_RAW(new_base - 2)._i = _stackbase; // push return frameptr
                        REGISTER_RAW(new_base - 1) = value_from_array(&closure->captures);
                        _pr = closure->func;
                        // _pc = func_start - 1; // jump to addr
                        _pc = -1; // TODO: gather all instructions into one big chunk to avoid this
                        _pe = _pr->instructions.size();
                        _stackbase = _stackbase + new_base; // new stack frame
                    }
                    handle(RANDOM) {
                        REGISTER(i.r0) = value_from_number(rand() % 10000);
                    }
                    handle(CLOCK) {
                        REGISTER(i.r0) = value_from_number(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count() / 1e6);
                    }
                    handle(RET) {
                        auto return_addr = REGISTER_RAW(-4);
                        auto return_func = REGISTER_RAW(-3);
                        auto return_stack= REGISTER_RAW(-2);

                        REGISTER_RAW(-4) = i.r0 ? REGISTER_RAW(i.r1) : value_null();
                        REGISTER_RAW(-3) = value_null();
                        REGISTER_RAW(-2) = value_null();
                        REGISTER_RAW(-1) = value_null();

                        // "Clean" the stack
                        // Must not leave any boxes in unused registers, or subsequent loads to register will mistakenly write-through
                        // TODO: try and elide this, or make more efficient
                        memset(_stack.data() + _stackbase, 0xffffffff, MAX_REGISTERS * sizeof(Value));
                        
                        _pc = return_addr._i;
                        _pr = (Program*)return_func._p;
                        _pe = _pr->instructions.size();
                        _stackbase = return_stack._i;

                        if (_stackbase == 0) {
                            // end of program
                            _pc = _pe;
                        }


                    }
                    handle(PRINT) {
                        log<true,false>(REGISTER(i.r0));
                    }
                    break; default: error("unknown instruction: " + to_string(i.opcode));
                }
                _pc++;
            }
        }
        catch (exception& e) {
            // TODO: stack unwind
            log("runtime error: "s, e.what());
        }
    }
};