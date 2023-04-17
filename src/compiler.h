#pragma once

#include <string>
#include <list>
#include <array>
#include <vector>

#include "instructions.h"
#include "value.h"

static const uint32_t MAX_REGISTERS = 256;
static const uint32_t STACK_FRAME_OVERHEAD = 3; // TODO: make this 3, can load captures through _pr
static const uint32_t MAX_STACK = 4096;

enum class RegisterState {
    FREE = 0,
    BUSY = 1,
    BOUND = 2
};

struct AstNode;
struct Interpreter;

struct CaptureInfo {
    uint8_t source_register;
    uint8_t dest_register;
};
struct CodeFragment {
    std::string name;
    std::vector<Instruction> instructions;
    std::vector<Value> storage; // program constant storage goes at the bottom of the stack for now
    std::list<std::string> strings; // string constants and literals storage - includes identifiers for objects
    std::vector<CaptureInfo> capture_info;

    uint16_t store_number(double d);
    uint16_t store_string(const std::string& data);
    uint16_t store_fragment(CodeFragment* ptr);
    std::string str();
};

struct Compiler {
    struct VariableContext {
        uint8_t reg = 0;
        bool is_const = false;
        bool is_global = false;
        uint16_t g_id = 0;
    };
    struct ScopeContext {
        Compiler* compiler;
        ScopeContext* parent_scope = nullptr;
        bool is_function_scope = false;
        std::unordered_map<std::string, VariableContext> bindings; // variables

        // lookup a variable
        // if it lives in a parent function, code will be emitted to capture it
        VariableContext* lookup(const std::string& name);
    };
    Interpreter* interpreter;
    // root AST node from last compile_func() call
    const AstNode* node;
    // current output as of last compile_func() call, included here for convenience
    CodeFragment* output;
    // bool is_global = false; // variable bindings become global instead of stack

    // compiler state
    std::array<RegisterState, MAX_REGISTERS> registers;
    std::list<ScopeContext> scopes;
    std::vector<CaptureInfo> captures;

    // lookup a variable in the current scope stack
    VariableContext* lookup(const std::string& name);

    // get a free register and mark as busy
    uint8_t allocate_register();
    // get 2 free registers; returns the first one; mark both as busy
    uint8_t allocate_register2();

    // get the register immediately after the highest non-free register
    uint8_t get_end_register();

    // set a register or global as bound by a variable with given name
    // NOTE: if it's global, make sure to emit WRITE_GLOBAL when relevant
    // this method can't currently do it automatically - see FuncLiteral compiler
    VariableContext* bind_name(const std::string& binding, uint8_t reg, bool is_const = false, bool is_export = false);
    
    // free a register if it's not bound
    void free_register(uint8_t reg);

    // free all unbound registers
    void free_all_registers();

    void push_scope(ScopeContext* parent_scope, bool is_top_level = false);
    void pop_scope();

    void compile_func(const AstNode* node, CodeFragment* output, ScopeContext* parent_scope = nullptr);
    uint8_t compile(const AstNode* node);

};


