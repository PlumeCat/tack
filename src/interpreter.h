#pragma once

#include "../include/tack.h"
#include "compiler.h"

#include <chrono>
#include <cstring>

// Hidden box type
struct BoxType {
    TackValue value;
    bool marker = false; // boxes don't need refcount
};
#define type_bits_boxed (0x00'0b'00'00'00'00'00'00)
static inline TackValue value_from_boxed(BoxType* box)             { return { nan_bits | type_bits_boxed | uint64_t(box) }; }
static inline bool value_is_boxed(TackValue v)                     { return std::isnan(v._d) && (v._i & type_bits) == type_bits_boxed; }
static inline BoxType* value_to_boxed(TackValue v)                 { return (BoxType*)(v._i & pointer_bits); }

struct Stack : std::array<TackValue, MAX_STACK> {
    uint32_t base = 0;

    inline void push_frame(TackValue::FunctionType* return_pr, uint32_t return_pc, uint8_t return_reg) {
        auto old_base = base;
        base += return_reg + STACK_FRAME_OVERHEAD;
        operator[](base - 3)._i = return_pc;
        operator[](base - 2)._p = (void*)return_pr;
        operator[](base - 1)._i = old_base;
    }
    inline void pop_frame(TackValue::FunctionType** out_pr, uint32_t* out_pc) {
        *out_pc = operator[](base-3)._i;
        *out_pr = (TackValue::FunctionType*)operator[](base-2)._p;
        auto return_base = operator[](base-1)._i;
        // "Clean" the stack - Must not leave any boxes in unused registers or subsequent loads to register will mistakenly write-through
        std::memset(data() + base - 2, 0xffffffff, MAX_REGISTERS * sizeof(TackValue));
        base = return_base;
    }
};
struct Heap {
private:
    // heap
    std::list<TackValue::ArrayType> arrays;
    std::list<TackValue::ObjectType> objects;
    std::list<TackValue::FunctionType> functions;
    std::list<BoxType> boxes;
    std::list<TackValue::StringType> strings; // temp strings - garbage collected
    
    // statistics
    std::chrono::steady_clock::time_point last_gc = std::chrono::steady_clock::now();
    uint32_t prev_alloc_count = 0;
    uint32_t alloc_count = 0; // a bit clumsy - counts all allocations
    float last_gc_ms = 0.f;
    TackGCState state = TackGCState::Enabled;

public:
    TackValue::ArrayType* alloc_array();
    TackValue::ObjectType* alloc_object();
    TackValue::FunctionType* alloc_function(CodeFragment* code);
    TackValue::FunctionType* alloc_function(TackValue::CFunctionType cfunction);
    BoxType* alloc_box(TackValue val);
    // makes copy of data
    TackValue::StringType* alloc_string(const std::string& data);

    TackGCState gc_state() const;
    void gc_state(TackGCState new_state);
    void gc(std::vector<TackValue>& globals, const Stack& stack, uint32_t stackbase);
};
class Interpreter: public TackVM {
    std::vector<std::string> module_dirs;
    
    Heap heap;
    Stack stack;
    uint32_t stackbase;
    std::vector<TackValue> globals;
    uint16_t next_globalid;
    KHash<std::string, TackValue::StringType*> key_cache; // TODO: move this into heap and use the refcount mechanism (and rename it!)

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
    TackGCState get_gc_state() const override;
    void set_gc_state(TackGCState state) override;

    inline void set_global(const std::string& name, TackValue value, bool is_const) override { set_global_v(name, value, is_const); }
    inline void set_global(const std::string& name, const std::string& module_name, TackValue value, bool is_const) override { set_global_v(name, module_name, value, is_const); }
    TackValue get_global(const std::string& name) override;
    TackValue get_global(const std::string& name, const std::string& module_name) override;

    void add_libs() override;
    void add_module_dir(const std::string& dir) override;
    inline void load_module(const std::string& module_name) override { load_module_s(module_name); }
    TackValue call(TackValue fn, int nargs, TackValue* args) override;
    void error(const std::string& msg) override;

    TackValue get_type_name(TackType type) override;
    
    TackValue::ArrayType* alloc_array() override;
    TackValue::ObjectType* alloc_object() override;
    TackValue::StringType* alloc_string(const std::string& data) override;
    TackValue::StringType* intern_string(const std::string& data) override;
    TackValue::FunctionType* alloc_function(CodeFragment* code);
    TackValue::FunctionType* alloc_function(TackValue::CFunctionType cfunction);

    CodeFragment* create_fragment();
    BoxType* alloc_box(TackValue val);
    void add_module_dir_cwd();
    Compiler::VariableContext* set_global_v(const std::string& name, TackValue value, bool is_const);
    Compiler::VariableContext* set_global_v(const std::string& name, const std::string& module_name, TackValue value, bool is_const);
    Compiler::ScopeContext* load_module_s(const std::string& filename);

private:
    bool parse(const std::string& code, AstNode& out_ast);
    uint16_t next_gid();    
};
