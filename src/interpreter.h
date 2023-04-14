#pragma once

#include "compiler.h"
#include "value.h"

struct Interpreter {
private:
    // Compiler root_compiler;
    Compiler::ScopeContext global_scope; // c-provided globals go here
    uint16_t next_gid; // TODO: bit clumsy, try and improve it
    friend class Compiler;

    std::vector<Value> globals;
    void execute(CodeFragment* program);

public:
    Interpreter();

    // equivalent to "let name = value"
    void set_global(const std::string& name, Value value);
    Value get_global(const std::string& name);
    void execute(const std::string& code, int argc = 0, char* argv[] = nullptr);
};