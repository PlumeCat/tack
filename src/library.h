#include "interpreter.h"

// helper macro for binding quick functions

#define tack_func_out(name, func) vm->set_global(name, true, value_from_cfunction(func));

#define tack_func(name, body) tack_func_out(name, \
    [](Interpreter* vm, int nargs, Value* args) -> Value {\
        body; \
        return value_null();\
    }\
);

void setup_standard_library(Interpreter* vm);
