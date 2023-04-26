#include "interpreter.h"

// helper macro for binding quick functions
#define tack_func(name, body) vm->set_global(name, true, value_from_cfunction(\
    [](Interpreter* vm, int nargs, Value* args) -> Value {\
        body; \
        return value_null();\
    }\
));

void setup_standard_library(Interpreter* vm);
