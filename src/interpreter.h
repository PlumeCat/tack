#pragma once

#include "compiler.h"

struct Interpreter {
    void execute(CompiledFunction* program);
};