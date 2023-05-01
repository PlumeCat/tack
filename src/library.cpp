#include "interpreter.h"
#include "library.h"

#include <sstream>
#include <jlib/log.h>
#include <jlib/text_file.h>

static const float pi = 3.1415926535f;
static const float degtorad = pi / 180.f;
static const float radtodeg = 180.f / pi;

double radians(double deg) {
    return deg * degtorad;
}
double degrees(double rad) {
    return rad * radtodeg;
}

template<typename ...Args> void print(const Args&... args) {
    log<true, false>(args...);
}

void setup_standard_library(Interpreter* vm) {
    tack_func("print", {

        auto ss = std::stringstream {};
        for (auto i = 0; i < nargs; i++) {
            ss << args[i] << ' ';
        }
        print(ss.str());
    });
    // });
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
    tack_func("tostring", {
        auto s = std::stringstream {};
        s << args[0];
        auto str = s.str();
        return value_from_string(vm->alloc_string(str));
    });

    // generic    
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
    #define tack_math(func) tack_func(#func, return value_from_number(func(value_to_number(args[0]))));
    #define tack_math2(func) tack_func(#func, return value_from_number(func(value_to_number(args[0]), value_to_number(args[1]))));

    vm->set_global("pi", true, value_from_number(pi));

    tack_math(sin);
    tack_math(cos);
    tack_math(tan);
    tack_math(asin);
    tack_math(acos);
    tack_math(atan);
    tack_math2(atan2);

    tack_math(sinh);
    tack_math(cosh);
    tack_math(tanh);
    tack_math(asinh);
    tack_math(acosh);
    tack_math(atanh);

    tack_math(exp);
    tack_math(exp2);
    tack_math(sqrt);
    tack_math(log);
    tack_math2(pow);
    tack_math(log2);
    tack_math(log10);

    tack_math(floor);
    tack_math(ceil);
    tack_math(abs);
    tack_math(round);
    tack_math(degrees);
    tack_math(radians);

    // fmod
    // frexp
    // ldexp
    // mod
    // frac
    // modf
    // pow
    // sign
    // smoothstep
    // smootherstep
    // lerp
    // slerp
    // clamp
    // atan2 in some form
    // min
    // max


    #undef tack_math
    #undef tack_math2


    // ===== OPERATORS =====
    // x array << x       => append x to array (returns x)
    // x #array           => length
    // x #string          => length
    // x #object          => length
    // array + array    => concat arrays
    // string + string  => concat strings
    // object + object  => union objects (right overrides left)
}

