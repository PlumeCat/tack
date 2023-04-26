#include "interpreter.h"
#include "library.h"

#include <jlib/log.h>

#define PRINT log<true,false>

void setup_standard_library(Interpreter* vm) {
    tack_func("dofile", {
        for (auto i = 0; i < nargs; i++) {
            auto s = value_to_string(args[i]);
            // vm->call(vm->load(read_text_file(s).value_or("")), nullptr, 0);
        }
    });
    tack_func("print", {
        auto ss = std::stringstream {};
        for (auto i = 0; i < nargs; i++) {
            ss << args[i] << ' ';
        }
        PRINT(ss.str());
    });
    tack_func("random", {
        return value_from_number(rand());
    });
    tack_func("clock", {
        return value_from_number(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1e6);
    });
    tack_func("gc_disable", vm->gc_state(GCState::Disabled));
    tack_func("gc_enable", vm->gc_state(GCState::Enabled));
    tack_func("read_file", {}); // read file to string
    tack_func("write_file", {}); // write string to file
    
    // generic
    tack_func("tostring", {});
    tack_func("type", {});
    tack_func("slice", {});
    tack_func("find", {});
    tack_func("join", {});

    // string
    tack_func("tonumber", {});
    tack_func("chr", {});
    tack_func("ord", {});
    tack_func("replace", {});
    tack_func("lower", {});
    tack_func("upper", {});
    tack_func("split", {});
    tack_func("format", {});

    // array/objects funcs
    tack_func("filter", {});
    tack_func("reduce", {});
    tack_func("map", {});
    tack_func("foreach", {});
    tack_func("keys", {});
    tack_func("values", {});
    /*
    insert
    push
    pop
    push_front
    pop_front
    sort
    */

    // math funcs


    // ===== OPERATORS =====
    // array << x       => append x to array (returns x)
    // #array           => length
    // #string          => length
    // #object          => length
    // array + array    => concat arrays
    // string + string  => concat strings
    // object + object  => union objects (right overrides left)
}

