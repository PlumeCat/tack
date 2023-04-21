#pragma once

#include "compiler.h"
#include "value.h"

struct Stack : std::array<Value, MAX_STACK> {
    // void push_frame(CodeFragment* current_pr, uint32_t current_pc, uint32_t current_sb, Value* newcap);
    // void pop_frame();
};
struct Heap {
private:
    // heap
    std::list<ArrayType> arrays;
    std::list<ObjectType> objects;
    std::list<FunctionType> functions;
    std::list<BoxType> boxes;
    
    // statistics
    std::chrono::steady_clock::time_point last_gc = std::chrono::steady_clock::now();
    uint32_t prev_alloc_count;
    uint32_t alloc_count; // a bit clumsy - counts all allocations

public:
    ArrayType* alloc_array();
    ObjectType* alloc_object();
    FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);

    void gc(std::vector<Value>& globals, const Stack& stack, uint32_t stackbase);
};

struct Interpreter {
private:
    Heap heap;
    Stack stack;
    uint32_t stackbase;

    Compiler::ScopeContext global_scope; // c-provided globals go here
    uint16_t next_globalid;
    std::vector<Value> globals;
    std::list<CodeFragment> fragments;

public:
    Interpreter();

    // used by Compiler
    CodeFragment* create_fragment();
    // equivalent to "export name = value"
    Compiler::VariableContext* set_global(const std::string& name, bool is_const, Value value);
    Value get_global(const std::string& name);
    uint16_t next_gid();

    // returns the file-level scope as a callable function
    Value load(const std::string& source);
    Value call(Value fn, Value* args, int nargs);
    
    // convenience wrapper
    template<size_t N>
    Value call(Value fn, std::array<Value, N>& args) {
        return call(fn, args.data(), N);
    }
};