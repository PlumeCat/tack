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
    
    // statistics
    std::chrono::steady_clock::time_point last_gc = std::chrono::steady_clock::now();
    uint32_t prev_alloc_count = 0;
    uint32_t alloc_count = 0; // a bit clumsy - counts all allocations
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
    std::vector<std::string> module_dirs;
    
    Heap heap;
    Stack stack;
    uint32_t stackbase;
    std::vector<Value> globals;
    uint16_t next_globalid;
    KHash<std::string, StringType*> key_cache; // TODO: move this into heap and use the refcount mechanism

    Compiler::ScopeContext global_scope; // c-provided globals go here
    KHash<std::string, Compiler::ScopeContext*> modules; // all loaded modules; "" is global module and is always implicitly imported
    std::list<CodeFragment> fragments;

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
    // used by host programs
    Compiler::VariableContext* set_global(const std::string& name, bool is_const, Value value);
    // used by Compiler from load_module (not to be used by host programs; TODO: make it private)
    Compiler::VariableContext* set_global(const std::string& name, const std::string& module_name, bool is_const, Value value);
    Value get_global(const std::string& name);
    Value get_global(const std::string& name, const std::string& module_name);
    uint16_t next_gid();

    // load a module and return the export scope context
    // note: the module will only ever run once per interpreter, if it was already loaded, the existing scope context is returned
    void add_module_dir_cwd();
    void add_module_dir(const std::string& dir);
    Compiler::ScopeContext* load_module(const std::string& module_name);    
    Value call(Value fn, int nargs, Value* args);

    GCState gc_state() const;
    void gc_state(GCState state);

    ArrayType* alloc_array();
    ObjectType* alloc_object();
    FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);

    // take a copy of data
    StringType* intern_string(const std::string& data);

    // take a copy of data
    StringType* alloc_string(const std::string& data);
};
