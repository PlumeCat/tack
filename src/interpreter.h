#pragma once

#include "../include/tack.h"
#include "compiler.h"

#include <chrono>

// Hidden box type
struct BoxType {
    Value value;
    uint32_t refcount = 0;
    bool marker = false;
};
#define type_bits_boxed (0x00'0b'00'00'00'00'00'00)
static inline Value value_from_boxed(BoxType* box)             { return { nan_bits | type_bits_boxed | uint64_t(box) }; }
static inline bool value_is_boxed(Value v)                     { return std::isnan(v._d) && (v._i & type_bits) == type_bits_boxed; }
static inline BoxType* value_to_boxed(Value v)                 { return (BoxType*)(v._i & pointer_bits); }

struct Stack : std::array<Value, MAX_STACK> {};
struct Heap {
private:
    // heap
    std::list<Value::ArrayType> arrays;
    std::list<Value::ObjectType> objects;
    std::list<Value::FunctionType> functions;
    std::list<BoxType> boxes;
    std::list<Value::StringType> strings; // temp strings - garbage collected
    
    // statistics
    std::chrono::steady_clock::time_point last_gc = std::chrono::steady_clock::now();
    uint32_t prev_alloc_count = 0;
    uint32_t alloc_count = 0; // a bit clumsy - counts all allocations
    float last_gc_ms = 0.f;
    GCState state = GCState::Enabled;

public:
    Value::ArrayType* alloc_array();
    Value::ObjectType* alloc_object();
    Value::FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);

    // takes ownership of data; data must be allocated with 'new[]'
    Value::StringType* alloc_string(const std::string& data);

    GCState gc_state() const;
    void gc_state(GCState new_state);

    void gc(std::vector<Value>& globals, const Stack& stack, uint32_t stackbase);
};
class Interpreter: public TackVM {
    std::vector<std::string> module_dirs;
    
    Heap heap;
    Stack stack;
    uint32_t stackbase;
    std::vector<Value> globals;
    uint16_t next_globalid;
    KHash<std::string, Value::StringType*> key_cache; // TODO: move this into heap and use the refcount mechanism

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

    void* get_user_pointer() const override;
    void set_user_pointer(void* ptr) override;

    void error(const std::string& msg) override;

    inline void set_global(const std::string& name, Value value, bool is_const) override {
        set_global_v(name, value, is_const);
    }
    inline void set_global(const std::string& name, const std::string& module_name, Value value, bool is_const) override {
        set_global_v(name, module_name, value, is_const);
    }
    inline void load_module(const std::string& module_name) override {
        load_module_s(module_name);
    }

    CodeFragment* create_fragment();
    Compiler::VariableContext* set_global_v(const std::string& name, Value value, bool is_const);
    Compiler::VariableContext* set_global_v(const std::string& name, const std::string& module_name, Value value, bool is_const);
    Value get_global(const std::string& name) override;
    Value get_global(const std::string& name, const std::string& module_name) override;
    uint16_t next_gid();
    void add_libs() override;
    void add_module_dir_cwd();
    void add_module_dir(const std::string& dir);
    Compiler::ScopeContext* load_module_s(const std::string& filename);
    Value call(Value fn, int nargs, Value* args);
    GCState get_gc_state() const;
    void set_gc_state(GCState state);
    Value::ArrayType* alloc_array();
    Value::ObjectType* alloc_object();
    Value::FunctionType* alloc_function(CodeFragment* code);
    BoxType* alloc_box(Value val);
    Value::StringType* alloc_string(const std::string& data);
    Value::StringType* intern_string(const std::string& data);

    bool parse(const std::string& code, AstNode& out_ast);
};
