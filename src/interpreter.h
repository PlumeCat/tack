#pragma once

#include "compiler.h"

struct Interpreter {
    void execute(CodeFragment* program);
};