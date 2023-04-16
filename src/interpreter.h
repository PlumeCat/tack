#pragma once

#include "compiler.h"
#include "value.h"

using StackType = std::array<Value, MAX_STACK>;
struct Heap {
private:
    // heap
    std::list<ArrayType> _arrays;
    std::list<ObjectType> _objects;
    std::list<FunctionType> _functions;
    std::list<BoxType> _boxes;
    
    // statistics
    std::chrono::steady_clock::time_point last_gc;
    std::chrono::steady_clock::duration last_gc_duration;

public:
    ArrayType* alloc_array();
    ObjectType* alloc_object();
    FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);

    void gc(std::vector<Value>& globals, const StackType& stack, uint32_t stackbase);
};

struct Interpreter {
private:
    Heap heap;
    Compiler::ScopeContext global_scope; // c-provided globals go here
    uint16_t next_globalid; // TODO: bit clumsy, try and improve it

    std::vector<Value> globals;
    std::list<CodeFragment> fragments;

    void execute(CodeFragment* fragment);

public:
    Interpreter();

    // used by Compiler
    CodeFragment* create_fragment();
    // equivalent to "export name = value"
    Compiler::VariableContext* set_global(const std::string& name, bool is_const, Value value);
    Value get_global(const std::string& name);
    uint16_t next_gid();
    void execute(const std::string& code, bool log_ast, bool log_bytecode);
};