#include "interpreter.h"
#include "library.h"

#include <sstream>
#define JLIB_LOG_VISUALSTUDIO
#include <jlib/log.h>
#include <jlib/text_file.h>

using namespace std::string_literals;
static const float pi = 3.1415926535f;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// TODO: string routines not optimal
std::string replace_all(const std::string& data, const std::string& s1, const std::string& s2) {
    auto s = std::stringstream {};
    auto pos = 0;
    auto n = s1.size();
    if (!n) {
        return data;
    }
    while (true) {
        auto f = data.find(s1, pos);
        if (f == std::string::npos) {
            // no more matches, append the rest of s
            s << std::string_view(data.begin() + pos, data.end());
            break;
        } else {
            // found match, append data[pos:f] and s2
            s << std::string_view(data.begin() + pos, data.begin() + f);
            s << std::string_view(s2);
            pos = f + n;
        }
    }
    return s.str();
}
std::vector<std::string> split_ws(const std::string& data) {
    auto retval = std::vector<std::string> {};

    auto pos = 0;
    auto ws = false;
    for (auto c = 0; c < data.size(); c++) {
        if (!ws && std::isspace(data[c])) {
            // found space
            ws = true;
            if (c - pos) retval.emplace_back(data.begin() + pos, data.begin() + c);
        } else if (ws && !std::isspace(data[c])) {
            ws = false;
            pos = c;
        }
    }
    if (!ws) retval.emplace_back(data.begin() + pos, data.end());
    return retval;
}
std::vector<std::string> split(const std::string& data, const std::string& split) {
    if (!split.size()) {
        // special case split whitespace
        return split_ws(data);
    }

    auto retval = std::vector<std::string> {};
    auto pos = 0;
    auto n = split.size();
    while (true) {
        auto f = data.find(split, pos);
        if (f == std::string::npos) {
            // no more matches
            if (data.begin() + pos != data.end()) retval.emplace_back(data.begin() + pos, data.end());
            break;
        } else {
            if (f != pos) retval.emplace_back(data.begin() + pos, data.begin() + f);
            pos = f + n;
        }
    }

    return retval;
}

//
#define error(msg) throw std::runtime_error(msg);
#define check_args(n) { if (nargs != n) { error("expected "s + std::to_string(n) + " arguments"); }}
#define tack_func_name(name) cat(tack_, name)
#define tack_func(name) Value tack_func_name(name)(Interpreter* vm, int nargs, Value* args)
#define tack_bind(name)  vm->set_global(#name, true, value_from_cfunction(tack_func_name(name)));
#define tack_math(func)  vm->set_global(#func, true, value_from_cfunction([](Interpreter* vm, int nargs, Value* args) { \
    check_args(1); \
    return value_from_number(func(value_to_number(args[0])));\
}));
#define tack_math2(func) vm->set_global(#func, true, value_from_cfunction([](Interpreter* vm, int nargs, Value* args) { \
    check_args(2);\
    return value_from_number(func(value_to_number(args[0]), value_to_number(args[1])));\
}));
#define tack_math3(func) vm->set_global(#func, true, value_from_cfunction([](Interpreter* vm, int nargs, Value* args) { \
    check_args(3);\
    return value_from_number(func(value_to_number(args[0]), value_to_number(args[1]), value_to_number(args[2]));\
}));


// STANDARD LIBRARY FUNCTIONS
tack_func(print) {
    for (auto i = 0; i < nargs; i++) {
        std::cout << value_get_string(args[i]) << ' ';
    }
    std::cout << std::endl;
    return value_null();
}
tack_func(random) {
    return value_from_number(rand());
}
tack_func(clock) {
    return value_from_number(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1e6);
}
tack_func(gc_disable) {
    vm->gc_state(GCState::Disabled);
    return value_null();
}
tack_func(gc_enable) {
    vm->gc_state(GCState::Enabled);
    return value_null();
}
tack_func(read_file) { return value_null(); } // read file to string
tack_func(write_file) { return value_null(); } // write string to file

// generic
tack_func(tostring) {
    check_args(1);
    return value_from_string(vm->alloc_string(value_get_string(args[0])));
}

// array funcs
tack_func(any) { return value_null(); }
tack_func(all) { return value_null(); }
tack_func(next) { return value_null(); }
tack_func(foreach) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    if (arr->size()) {
        if (value_is_function(args[1])) {
            auto func = args[1];
            for (auto i = 0; i < arr->size(); i++) {
                vm->call(func, 1, &arr->at(i));
            }
        } else if (value_is_cfunction(args[1])) {
            auto func = value_to_cfunction(args[1]);
            for (auto i = 0; i < arr->size(); i++) {
                func(vm, 1, &arr->at(i));
            }
        }
    }
    return value_null();
}
tack_func(map) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    auto retval = vm->alloc_array();
    retval->refcount = 1;
    if (arr->size()) {
        retval->resize(arr->size(), value_null());
        if (value_is_function(args[1])) {
            auto func = args[1];
            for (auto i = 0; i < arr->size(); i++) {
                retval->at(i) = vm->call(func, 1, &arr->at(i));
            }
        } else if (value_is_cfunction(args[1])) {
            auto func = value_to_cfunction(args[1]);
            for (auto i = 0; i < arr->size(); i++) {
                retval->at(i) = func(vm, 1, &arr->at(i));
            }
        }
    }
    retval->refcount = 0;
    return value_from_array(retval);
}
tack_func(filter) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    auto* retval = vm->alloc_array();
    retval->refcount = 1;
    auto func = args[1];
    if (value_is_function(func)) {
        for (auto i = arr->begin(); i != arr->end(); i++) {
            auto val = *i;
            if (value_get_truthy(vm->call(func, 1, &val))) {
                retval->emplace_back(val);
            }
        }
    } else if (value_is_cfunction(func)) {
        auto* f = value_to_cfunction(func);
        for (auto val : *arr) {
            if (value_get_truthy(f(vm, 1, &val))) {
                retval->push_back(val);
            }
        }
    } else {
        error("expected function or cfunction in filter()");
    }
    retval->refcount = 0;
    return value_from_array(retval);
}
tack_func(reduce) {
    check_args(3);
    auto* arr = value_to_array(args[0]);
    auto func = args[1];
    if (value_is_function(func)) {
        auto ra = std::array<Value, 2>{}; // arguments for reducer function
        ra[1] = args[2];
        for (auto i = arr->begin(); i != arr->end(); i++) {
            ra[0] = *i;
            ra[1] = vm->call(func, 2, ra.data());
        }
        return ra[1];
    } else if (value_is_cfunction(func)) {
        auto* f = value_to_cfunction(func);
        auto ra = std::array<Value, 2>{}; // arguments for reducer function
        ra[1] = args[2];
        for (auto i = arr->begin(); i != arr->end(); i++) {
            ra[0] = *i;
            ra[1] = f(vm, 2, ra.data());
        }
        return ra[1];
    } else {
        error("expected function or cfunction in filter()");
    }
}
tack_func(sort) {
    // TODO: currently assumes the type by looking at the first elemen { return value_null(); }
    // behaviour will be undefined for a mixed-type array (and no custom predicate)
    auto retval = vm->alloc_array();
    retval->refcount = 1;
    if (nargs == 1) {
        auto* arr = value_to_array(args[0]);
        if (arr->size()) {
            std::copy(arr->begin(), arr->end(), std::back_inserter(*retval));
            auto type = value_get_type(arr->at(0));
            if (type == Type::Number) {
                // default number sort
                std::sort(retval->begin(), retval->end(), [](Value l, Value r) {
                    return value_to_number(l) < value_to_number(r);
                });
            } else if (type == Type::String) {
                // default string sort
                std::sort(retval->begin(), retval->end(), [](Value l, Value r) {
                    return value_to_string(l)->data < value_to_string(r)->data;
                });
            } else {
                error("sort: expects an array of number or an array of string");
            }
        }
    } else if (nargs == 2) {
        auto* arr = value_to_array(args[0]);
        if (arr->size()) {
            std::copy(arr->begin(), arr->end(), std::back_inserter(*retval));
            auto func = args[1];
            if (value_is_function(func)) {
                // custom sort with tack function
                std::sort(retval->begin(), retval->end(), [&](Value l, Value r) {
                    // return true;
                    Value lr[2] = { l, r };
                    return value_to_boolean(vm->call(func, 2, lr));
                });
            } else if (value_is_cfunction(func)) {
                auto f = value_to_cfunction(func);
                // custom sort with c function
                std::sort(retval->begin(), retval->end(), [&](Value l, Value r) {
                    // return true;
                    Value lr[2] = { l, r };
                    return value_to_boolean(f(vm, 2, lr));
                });
            } else {
                error("sort: expected function or cfunction for optional second argument");
            }
        }
    } else {
        error("sort expected 1 or 2 arguments");
    }

    retval->refcount = 0;
    return value_from_array(retval);
}
tack_func(sum) {
    check_args(1);
    auto* arr = value_to_array(args[0]);
    auto sum = 0.f;
    for (auto i : *arr) {
        sum += value_to_number(i);
    }
    return value_from_number(sum);
}

tack_func(push) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    auto val = args[1];
    arr->push_back(val);
    return value_null();
}
tack_func(push_front) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    auto val = args[1];
    if (arr->size()) {
        arr->insert(arr->begin(), val);
    } else {
        arr->emplace_back(val);
    }
    return value_null();
}
tack_func(pop) {
    check_args(1);
    auto* arr = value_to_array(args[0]);
    auto val = args[1];
    if (arr->size()) {
        auto val = arr->back();
        arr->pop_back();
        return val;
    }
    return value_null();
}
tack_func(pop_front) {
    check_args(1);
    auto* arr = value_to_array(args[0]);
    if (arr->size()) {
        auto val = *arr->begin();
        arr->erase(arr->begin());
        return val;
    }
    return value_null();
}

tack_func(insert) {
    check_args(3);
    auto* arr = value_to_array(args[0]);
    auto val = args[1];
    auto pos = value_to_number(args[2]);
    if (pos > arr->size()) {
        error("Insert out of bounds");
    }
    arr->insert(arr->begin() + pos, val);
    return value_null();
}
tack_func(remove) {
    if (nargs == 2) {
        auto* arr = value_to_array(args[0]);
        auto pos = (size_t)value_to_number(args[1]);
        if (pos < arr->size()) {
            arr->erase(arr->begin() + pos);
        }
    } else if (nargs == 3) {
        auto* arr = value_to_array(args[0]);
        auto s = arr->size();
        if (s) {
            auto i1 = arr->begin() + std::min((size_t)value_to_number(args[1]), s);
            auto i2 = arr->begin() + std::min((size_t)value_to_number(args[2]), s);
            arr->erase(i1, i2);
        }
    } else {
        error("expected "s + std::to_string(2) + " arguments");;
    }
    return value_null();
}
tack_func(remove_value) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    std::erase(*arr, args[1]);
    return value_null();
}
tack_func(remove_if) {
    check_args(2);
    auto* arr = value_to_array(args[0]);
    auto func = args[1];
    if (value_is_function(func)) {
        std::erase_if(*arr, [&](Value val) {
            return value_to_boolean(vm->call(func, 1, &val));
        });
    } else if (value_is_cfunction(func)) {
        auto* f = value_to_cfunction(func);
        std::erase_if(*arr, [&](Value val) {
            return value_to_boolean(f(vm, 1, &val));
        });
    } else {
        error("expected function or cfunction in filter()");
    }
    return value_null();
}
tack_func(min) {
    if (nargs == 0) {
        return value_null();
    } else if (nargs == 1) {
        auto arr = value_to_array(args[0]);
        if (!arr->size()) {
            return value_null();
        }
        auto m = value_to_number((*arr)[0]);
        for (auto i = 1u; i < arr->size(); i++) {
            m = std::min(m, value_to_number((*arr)[i]));
        }
        return value_from_number(m);
    } else {
        auto m = value_to_number(args[0]);
        for (auto i = 1; i < nargs; i++) {
            m = std::min(m, value_to_number(args[i]));
        }
        return value_from_number(m);
    }
}
tack_func(max) {
    if (nargs == 0) {
        return value_null();
    } else if (nargs == 1) {
        auto arr = value_to_array(args[0]);
        if (!arr->size()) {
            return value_null();
        }
        auto m = value_to_number((*arr)[0]);
        for (auto i = 1u; i < arr->size(); i++) {
            m = std::max(m, value_to_number((*arr)[i]));
        }
        return value_from_number(m);
    } else {
        auto m = value_to_number(args[0]);
        for (auto i = 1; i < nargs; i++) {
            m = std::max(m, value_to_number(args[i]));
        }
        return value_from_number(m);
    }
}
tack_func(join) {
    check_args(2);

    auto retval = std::stringstream {};

    auto* arr = value_to_array(args[0]);
    auto* str = value_to_string(args[1]);

    if (arr->size()) {
        for (auto i = arr->begin() + 1; i != arr->end(); i++) {
            retval << str->data << value_get_string(*i);
        }
    }

    return value_from_string(vm->alloc_string(retval.str()));
}

// array and string both
tack_func(slice) {
    check_args(3);
    auto l = (int)value_to_number(args[1]);
    auto r = (int)value_to_number(args[2]);

    if (value_is_array(args[0])) {
        auto* arr = value_to_array(args[0]);
        auto s = arr->size();
        if (l < 0 || l > s) {
            error("slice index out of range");
        }
        if (r < 0 || r > s) {
            error("slice index out of range");
        }
        auto ret = vm->alloc_array();
        ret->reserve(r - l);
        for (auto i = l; i < r; i++) {
            ret->emplace_back((*arr)[i]);
        }
        return value_from_array(ret);
    } else if (value_is_string(args[0])) {
        auto* str = value_to_string(args[0]);
        auto s = str->data.size();
        if (l < 0 || l > s) {
            error("slice index out of range");
        }
        if (r < 0 || r > s) {
            error("slice index out of range");
        }
        auto ret = vm->alloc_string(str->data.substr(l, r - l));
        return value_from_string(ret);
    } else {
        error("slice expected array or string");
    }
}
tack_func(find) {
    if (nargs < 2 || nargs > 3) {
        error("find expected 2 or 3 arguments");
    }
    auto offset = (nargs == 3) ? (int)value_to_number(args[2]) : 0;
    if (value_is_array(args[0])) {
        auto* arr = value_to_array(args[0]);
        auto val = args[1];
        for (auto i = 0; i < arr->size(); i++) {
            if (arr->at(i) == val) {
                return value_from_number(i);
            }
        }
        return value_from_number(-1);
    } else if (value_is_string(args[0])) {
        auto* str = value_to_string(args[0]);
        auto* sub = value_to_string(args[1]);
        auto f = str->data.find(sub->data, offset);
        if (f == std::string::npos) {
            return value_from_number(-1);
        }
        return value_from_number(f);
    } else {
        error("find expected array or string");
    }
}

// string
tack_func(chr) {
    // integer to character
    check_args(1);
    return value_from_string(vm->alloc_string(std::string(1, (char)value_to_number(args[0]))));
}
tack_func(ord) {
    // character to integer
    check_args(1);
    auto str = value_to_string(args[0]);
    auto ch = str->data.size() ? str->data[0] : 0.0;
    return value_from_number((char)ch);
}
tack_func(tonumber) {
    // TODO: ignoring leading whitespace and trailing characters
    // eg "123.45foo" -> 123.45
    // is this desired behaviour
    check_args(1);
    auto* str = value_to_string(args[0]);
    auto* data = str->data.data();
    auto* end = (char*)nullptr;
    auto retval = std::strtod(data, &end);
    auto dist = end - data;
    if (dist != 0) {
        return value_from_number(retval);
    }
    error("could not convert to number");
}
tack_func(tolower) {
    check_args(1);
    auto* str = vm->alloc_string(value_to_string(args[0])->data);
    for (auto& c : str->data) {
        c = std::tolower(c);
    }
    return value_from_string(str);
}
tack_func(toupper) {
    check_args(1);
    auto* str = vm->alloc_string(value_to_string(args[0])->data);
    for (auto& c : str->data) {
        c = std::toupper(c);
    }
    return value_from_string(str);
}
tack_func(split) {
    // TODO: inefficient
    auto retval = std::vector<std::string> {};
    if (nargs == 1) {
        retval = split_ws(value_to_string(args[0])->data);
    } else {
        if (nargs != 2) {
            error("expected "s + std::to_string(2) + " arguments");;
        }
        auto* sep = value_to_string(args[1]);
        if (!sep->data.size()) {
            error("empty split() separator");
        }
        retval = split(value_to_string(args[0])->data, sep->data);
    }
    auto* arr = vm->alloc_array();
    for (auto& s : retval) {
        arr->emplace_back(value_from_string(vm->alloc_string(s)));
    }
    return value_from_array(arr);
}
tack_func(replace) {
    check_args(3);
    auto* str = value_to_string(args[0]);
    auto* sub = value_to_string(args[1]);
    auto* rep = value_to_string(args[2]);
    return value_from_string(vm->alloc_string(replace_all(str->data, sub->data, rep->data)));
}
tack_func(isupper) { return value_null(); }
tack_func(islower) { return value_null(); }
tack_func(isdigit) { return value_null(); }
tack_func(isalnum) { return value_null(); }

// object funcs
tack_func(keys) { return value_null(); }
tack_func(values) { return value_null(); }
tack_func(items) { return value_null(); }

// numeric
tack_func(radtodeg) {
    check_args(1);
    return value_from_number(value_to_number(args[0]) * 180.f / pi);
}
tack_func(degtorad) {
    check_args(1);
    return value_from_number(value_to_number(args[0]) * pi / 180.f);
}
tack_func(lerp) {
    check_args(3);
    auto a = value_to_number(args[0]);
    auto b = value_to_number(args[1]);
    auto c = value_to_number(args[2]);
    return value_from_number(a + (b - a) * c);
}
tack_func(clamp) {
    check_args(3);
    return value_from_number(std::max(value_to_number(args[2]), std::min(value_to_number(args[1]), value_to_number(args[0]))));
}
tack_func(saturate) {
    check_args(1);
    return value_from_number(std::max(0.0, std::min(1.0, value_to_number(args[0]))));
}

// TODO: more interpolation: smoothstep/cubic, beziers, ...

void setup_standard_library(Interpreter* vm) {
    // generic
    tack_bind(print);
    tack_bind(random);
    tack_bind(clock);
    tack_bind(gc_disable);
    tack_bind(gc_enable);
    tack_bind(read_file);
    tack_bind(write_file);
    tack_bind(tostring);
    
    // array
    tack_bind(any);
    tack_bind(all);
    tack_bind(next);
    tack_bind(foreach);
    tack_bind(map);
    tack_bind(filter);
    tack_bind(reduce);
    tack_bind(sort);
    tack_bind(sum);
    tack_bind(push);
    tack_bind(push_front);
    tack_bind(pop);
    tack_bind(pop_front);
    tack_bind(insert);
    tack_bind(remove);
    tack_bind(remove_value);
    tack_bind(remove_if);
    tack_bind(min);
    tack_bind(max);
    tack_bind(join);
    
    // string or array
    tack_bind(slice);
    tack_bind(find);

    // string
    tack_bind(chr);
    tack_bind(ord);
    tack_bind(tonumber);
    tack_bind(split);
    tack_bind(replace);
    tack_bind(tolower);
    tack_bind(toupper);
    tack_bind(isupper);
    tack_bind(islower);
    tack_bind(isdigit);
    tack_bind(isalnum);

    // object
    tack_bind(keys);
    tack_bind(values);
    tack_bind(items);
    
    // math
    tack_bind(radtodeg);
    tack_bind(degtorad);
    tack_bind(lerp);
    tack_bind(clamp);
    tack_bind(saturate);

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
    tack_math2(fmod);    
    
    // pow
    // sign
    // smoothstep
    // smootherstep
    // slerp

    // ===== OPERATORS =====
    // x array << x       => append x to array (returns x)
    // x #array           => length
    // x #string          => length
    // x #object          => length
    // array + array    => concat arrays
    // string + string  => concat strings
    // object + object  => union objects (right overrides left)
}


#pragma GCC diagnostic pop
