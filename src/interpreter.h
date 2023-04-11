#pragma once

#include "compiler.h"
#include "value.h"

#include <jlib/hash_map.h>

struct Interpreter {
private:
    void execute(CodeFragment* program);

public:
    // equivalent to "let name = value"
    void set_global(const std::string& name, Value value);
    void execute(const std::string& code, int argc = 0, char* argv[] = nullptr);
};