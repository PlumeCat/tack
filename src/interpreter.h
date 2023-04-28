#pragma once

#include "compiler.h"
#include "library.h"
#include "value.h"

#include <chrono>

struct Stack : std::array<Value, MAX_STACK> {
    // void push_frame(CodeFragment* current_pr, uint32_t current_pc, uint32_t current_sb, Value* newcap);
    // void pop_frame();
};

enum class GCState : uint8_t {
    Disabled = 0,
    Enabled = 1,
};
struct Heap {
private:
    // heap
    std::list<ArrayType> arrays;
    std::list<ObjectType> objects;
    std::list<FunctionType> functions;
    std::list<BoxType> boxes;
    std::list<StringType> strings; // temp strings - garbage collected
    std::list<StringType> keys; // object keys and string literals - live forever // TODO: deduplicate these
    
    // statistics
    std::chrono::steady_clock::time_point last_gc = std::chrono::steady_clock::now();
    uint32_t prev_alloc_count;
    uint32_t alloc_count; // a bit clumsy - counts all allocations
    float last_gc_ms = 0.f;
    GCState state = GCState::Enabled;

public:
    ArrayType* alloc_array();
    ObjectType* alloc_object();
    FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);

    // takes ownership of data; data must be allocated with 'new[]'
    StringType* alloc_string(const std::string& data);

    GCState gc_state() const;
    void gc_state(GCState new_state);

    void gc(std::vector<Value>& globals, const Stack& stack, uint32_t stackbase);
};
struct Interpreter {
    bool log_ast = false;
    bool log_bytecode = false;

private:
    Heap heap;
    Stack stack;
    uint32_t stackbase;

    Compiler::ScopeContext global_scope; // c-provided globals go here
    uint16_t next_globalid;
    std::vector<Value> globals;
    std::list<CodeFragment> fragments;

    KHash<std::string, StringType*> key_cache;

    void* user_pointer = nullptr;

public:
    Interpreter();
    ~Interpreter();

    Interpreter(const Interpreter&) = delete;
    Interpreter(Interpreter&&) = delete;
    Interpreter& operator=(const Interpreter&) = delete;
    Interpreter& operator=(Interpreter&&) = delete;

    void* get_user_pointer() const;
    void set_user_pointer(void* ptr);

    // used by Compiler
    CodeFragment* create_fragment();
    // equivalent to "export name = value"
    Compiler::VariableContext* set_global(const std::string& name, bool is_const, Value value);
    Value get_global(const std::string& name);
    uint16_t next_gid();

    // returns the file-level scope as a callable function
    Value load(const std::string& source);
    Value call(Value fn, Value* args, int nargs);

    GCState gc_state() const;
    void gc_state(GCState state);
    
    // convenience wrapper
    template<size_t N>
    Value call(Value fn, std::array<Value, N>& args) {
        return call(fn, args.data(), N);
    }

    ArrayType* alloc_array();
    ObjectType* alloc_object();
    FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);

    // take a copy of data
    StringType* intern_string(const std::string& data);

    // take a copy of data
    StringType* alloc_string(const std::string& data);
};
