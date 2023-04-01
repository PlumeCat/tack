#pragma once

#include <list>
#include <array>
#include <vector>

#include "parsing.h"
#include "program.h"
#include "value.h"

enum class RegisterState {
    FREE = 0,
    BUSY = 1,
    BOUND = 2
};


static const uint32_t MAX_REGISTERS = 256;
static const uint32_t STACK_FRAME_OVERHEAD = 4;

struct FunctionCompiler {
    struct VariableContext {
        uint8_t reg;
        bool is_const = false;
    };
    struct ScopeContext {
        FunctionCompiler* compiler;
        ScopeContext* parent_scope = nullptr; // establishes a tree structure over FunctionContext::scopes
        bool is_top_level_scope = false;
        hash_map<std::string, VariableContext> bindings = hash_map<std::string, VariableContext>(1, 1); // variables

        // lookup a variable
        // if it lives in a parent function, code will be emitted to capture it
        VariableContext* lookup(const std::string& name, CompiledFunction* output);
    };
    // AST node to compile
    // TODO: make it not a parameter
    const AstNode* node;
    // name of function (if any)
    std::string name;

    // compiler state
    std::array<RegisterState, MAX_REGISTERS> registers;
    std::list<ScopeContext> scopes;
    std::vector<CaptureInfo> captures;

    // lookup a variable in the current scope stack
    VariableContext* lookup(const std::string& name, CompiledFunction* output);

    // get a free register
    uint8_t allocate_register();

    // get the register immediately after the highest non-free register
    uint8_t get_end_register();

    // set a register as bound by a variable
    VariableContext* bind_register(const std::string& binding, uint8_t reg, bool is_const = false);
    
    // free a register if it's not bound
    void free_register(uint8_t reg);

    // free all unbound registers
    void free_all_registers();

    void push_scope(ScopeContext* parent_scope, bool is_top_level = false);
    void pop_scope();

    void compile_func(const AstNode& node, CompiledFunction* output, ScopeContext* parent_scope = nullptr);
    uint8_t compile(const AstNode& node, CompiledFunction* output);

};



