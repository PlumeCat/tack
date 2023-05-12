#include "interpreter.h"

#include <sstream>
#include <fstream>
#include <optional>

using namespace std::string_literals;
static const float pi = 3.1415926535f;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// TODO: string routines not optimal
std::optional<std::string> read_text_file(const std::string& fname) {
    auto file = std::ifstream(fname);
    if (!file.is_open()) {
        return std::nullopt;
    }
    //auto start = file.tellg();
    //file.seekg(0, std::ios::end);
    //auto end = file.tellg();
    //auto length = (size_t)(end - start);
    //file.seekg(0, std::ios::beg);

    file.ignore(std::numeric_limits<std::streamsize>::max());
    std::streamsize length = file.gcount();
    file.clear();   //  Since ignore will have set eof.
    file.seekg(0, std::ios_base::beg);

    auto data = std::string(length, 0);
    file.read(data.data(), length);
    return std::optional(data);
}
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
    for (auto c = 0u; c < data.size(); c++) {
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
    auto pos = 0u;
    auto n = split.size();
    while (true) {
        auto f = data.find(split, pos);
        if (f == std::string::npos) {
            // no more matches
            if (data.begin() + pos != data.end()) retval.emplace_back(data.begin() + pos, data.end());
            break;
        } else {
            if (f != pos) {
                retval.emplace_back(data.begin() + pos, data.begin() + f);
            }
            pos = f + n;
        }
    }

    return retval;
}

#define paste(a, b) a##b
#define check_args(n) { if (nargs != n) { vm->error("expected "s + std::to_string(n) + " arguments"); }}
#define check_arg(i, ty) if (!(args[i].is_##ty())) vm->error("type error: expected " #ty);
#define check_val(v, ty) if (!((v).is_##ty())) vm->error("type error: expected " #ty);
#define tack_func_name(name) paste(tack_, name)
#define tack_func(name) TackValue tack_func_name(name)(TackVM* vm, int nargs, TackValue* args)
#define tack_bind(name)  vm->set_global(#name, TackValue::cfunction(tack_func_name(name)), true);
#define tack_math(func)  vm->set_global(#func, TackValue::cfunction([](TackVM* vm, int nargs, TackValue* args) { \
    check_args(1); \
    check_arg(0, number); \
    return TackValue::number(func(args[0].number()));\
}), true);
#define tack_math2(func) vm->set_global(#func, TackValue::cfunction([](TackVM* vm, int nargs, TackValue* args) { \
    check_args(2);\
    check_arg(0, number);\
    check_arg(1, number);\
    return TackValue::number(func(args[0].number(), args[1].number()));\
}), true);
#define tack_math3(func) vm->set_global(#func, TackValue::cfunction([](TackVM* vm, int nargs, TackValue* args) { \
    check_args(3);\
    check_arg(0, number);\
    check_arg(1, number);\
    check_arg(2, number);\
    return TackValue::number(func(args[0].number(), args[1].number(), args[2].number());\
}), true);

// standard library
tack_func(print) {
    for (auto i = 0; i < nargs; i++) {
        std::cout << args[i].get_string() << ' ';
    }
    std::cout << std::endl;
    return TackValue::null();
}
tack_func(random) {
    return TackValue::number(rand());
}
tack_func(clock) {
    return TackValue::number(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count() / 1e6);
}
tack_func(gc_disable) {
    vm->set_gc_state(TackGCState::Disabled);
    return TackValue::null();
}
tack_func(gc_enable) {
    vm->set_gc_state(TackGCState::Enabled);
    return TackValue::null();
}
// tack_func(read_file)
// tack_func(write_file)
// tack_func(read)
tack_func(tostring) {
    check_args(1);
    return TackValue::string(vm->alloc_string(args[0].get_string()));
}

// array funcs
tack_func(any) {
    check_args(1);
    check_arg(0, array);
    auto* arr = args[0].array();
    if (arr->data.size()) {
        for (auto i = 0u; i < arr->data.size(); i++) {
            if (arr->data.at(i).get_truthy()) {
                return TackValue::true_();
            }
        }
    }
    return TackValue::false_();
}
tack_func(all) {
    check_args(1);
    check_arg(0, array);
    auto* arr = args[0].array();
    if (arr->data.size()) {
        for (auto i = 0u; i < arr->data.size(); i++) {
            if (!arr->data.at(i).get_truthy()) {
                return TackValue::false_();
            }
        }
    }
    return TackValue::true_();
}
tack_func(next) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    if (arr->data.size()) {
        if (args[1].is_function()) {
            auto func = args[1];
            for (auto i = 0u; i < arr->data.size(); i++) {
                auto val = arr->data.at(i);
                auto res = vm->call(func, 1, &val);
                if (res.get_truthy()) {
                    return val;
                }
            }
        } else if (args[1].is_cfunction()) {
            auto func = args[1].cfunction();
            for (auto i = 0u; i < arr->data.size(); i++) {
                auto val = arr->data.at(i);
                auto res = (*func)(vm, 1, &val);
                if (res.get_truthy()) {
                    return val;
                }
            }
        }
    }
    return TackValue::null();
}
tack_func(foreach) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    if (arr->data.size()) {
        if (args[1].is_function()) {
            auto func = args[1];
            for (auto i = 0u; i < arr->data.size(); i++) {
                vm->call(func, 1, &arr->data.at(i));
            }
        } else if (args[1].is_cfunction()) {
            auto func = args[1].cfunction();
            for (auto i = 0u; i < arr->data.size(); i++) {
                (*func)(vm, 1, &arr->data.at(i));
            }
        }
    }
    return TackValue::null();
}
tack_func(map) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto retval = vm->alloc_array();
    retval->refcount = 1;
    if (arr->data.size()) {
        retval->data.resize(arr->data.size(), TackValue::null());
        if (args[1].is_function()) {
            auto func = args[1];
            for (auto i = 0u; i < arr->data.size(); i++) {
                retval->data.at(i) = vm->call(func, 1, &arr->data.at(i));
            }
        } else if (args[1].is_cfunction()) {
            auto func = args[1].cfunction();
            for (auto i = 0u; i < arr->data.size(); i++) {
                retval->data.at(i) = (*func)(vm, 1, &arr->data.at(i));
            }
        }
    }
    retval->refcount = 0;
    return TackValue::array(retval);
}
tack_func(filter) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto* retval = vm->alloc_array();
    retval->refcount = 1;
    auto func = args[1];
    if (func.is_function()) {
        for (auto i = arr->data.begin(); i != arr->data.end(); i++) {
            auto val = *i;
            if (vm->call(func, 1, &val).get_truthy()) {
                retval->data.emplace_back(val);
            }
        }
    } else if (func.is_cfunction()) {
        auto* f = func.cfunction();
        for (auto val : arr->data) {
            if (f(vm, 1, &val).get_truthy()) {
                retval->data.push_back(val);
            }
        }
    } else {
        vm->error("expected function or cfunction in filter()");
    }
    retval->refcount = 0;
    return TackValue::array(retval);
}
tack_func(reduce) {
    check_args(3);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto func = args[1];
    if (func.is_function()) {
        auto ra = std::array<TackValue, 2>{}; // arguments for reducer function
        ra[1] = args[2];
        for (auto i = arr->data.begin(); i != arr->data.end(); i++) {
            ra[0] = *i;
            ra[1] = vm->call(func, 2, ra.data());
        }
        return ra[1];
    } else if (func.is_cfunction()) {
        auto* f = func.cfunction();
        auto ra = std::array<TackValue, 2>{}; // arguments for reducer function
        ra[1] = args[2];
        for (auto i = arr->data.begin(); i != arr->data.end(); i++) {
            ra[0] = *i;
            ra[1] = f(vm, 2, ra.data());
        }
        return ra[1];
    } else {
        vm->error("expected function or cfunction in reduce()");
    }
    return TackValue::null();
}
tack_func(sort) {
    // TODO: currently assumes the type by looking at the first elemen { return TackValue::null(); }
    // behaviour will be undefined for a mixed-type array (and no custom predicate)
    auto retval = vm->alloc_array();
    retval->refcount = 1;
    if (nargs == 1) {
        check_arg(0, array);
        auto* arr = args[0].array();
        if (arr->data.size()) {
            std::copy(arr->data.begin(), arr->data.end(), std::back_inserter(retval->data));
            auto type = arr->data.at(0).get_type();
            if (type == TackType::Number) {
                // default number sort
                std::sort(retval->data.begin(), retval->data.end(), [vm](TackValue l, TackValue r) {
                    check_val(l, number);
                    check_val(r, number);
                    return l.number() < r.number();
                });
            } else if (type == TackType::String) {
                // default string sort
                std::sort(retval->data.begin(), retval->data.end(), [vm](TackValue l, TackValue r) {
                    check_val(l, string);
                    check_val(r, string);
                    return l.string()->data < r.string()->data;
                });
            } else {
                vm->error("sort: expects an array of number or an array of string");
            }
        }
    } else if (nargs == 2) {
        check_arg(0, array);
        auto* arr = args[0].array();
        if (arr->data.size()) {
            std::copy(arr->data.begin(), arr->data.end(), std::back_inserter(retval->data));
            auto func = args[1];
            if (func.is_function()) {
                // custom sort with tack function
                std::sort(retval->data.begin(), retval->data.end(), [&](TackValue l, TackValue r) {
                    TackValue lr[2] = { l, r };
                    return vm->call(func, 2, lr).get_truthy();
                });
            } else if (func.is_cfunction()) {
                auto f = func.cfunction();
                // custom sort with c function
                std::sort(retval->data.begin(), retval->data.end(), [&](TackValue l, TackValue r) {
                    TackValue lr[2] = { l, r };
                    return f(vm, 2, lr).get_truthy();
                });
            } else {
                vm->error("sort: expected function or cfunction for optional second argument");
            }
        }
    } else {
        vm->error("sort expected 1 or 2 arguments");
    }

    retval->refcount = 0;
    return TackValue::array(retval);
}
tack_func(sum) {
    check_args(1);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto sum = 0.f;
    for (auto i : arr->data) {
        check_val(i, number);
        sum += (i).number();
    }
    return TackValue::number(sum);
}

tack_func(push) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto val = args[1];
    arr->data.push_back(val);
    return TackValue::null();
}
tack_func(push_front) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto val = args[1];
    if (arr->data.size()) {
        arr->data.insert(arr->data.begin(), val);
    } else {
        arr->data.emplace_back(val);
    }
    return TackValue::null();
}
tack_func(pop) {
    check_args(1);
    check_arg(0, array);
    auto* arr = args[0].array();
    if (arr->data.size()) {
        auto val = arr->data.back();
        arr->data.pop_back();
        return val;
    }
    return TackValue::null();
}
tack_func(pop_front) {
    check_args(1);
    check_arg(0, array);
    auto* arr = args[0].array();
    if (arr->data.size()) {
        auto val = *arr->data.begin();
        arr->data.erase(arr->data.begin());
        return val;
    }
    return TackValue::null();
}

tack_func(insert) {
    check_args(3);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto val = args[1];
    check_arg(2, number);
    auto pos = (args[2]).number();
    if (pos > arr->data.size()) {
        vm->error("Insert out of bounds");
    }
    arr->data.insert(arr->data.begin() + pos, val);
    return TackValue::null();
}
tack_func(remove) {
    if (nargs == 2) {
        check_arg(0, array);
        check_arg(1, number);
        auto* arr = args[0].array();
        auto pos = (size_t)(args[1].number());
        if (pos < arr->data.size()) {
            arr->data.erase(arr->data.begin() + pos);
        }
    } else if (nargs == 3) {
        check_arg(0, array);
        check_arg(1, number);
        check_arg(2, number);
        auto* arr = args[0].array();
        auto s = arr->data.size();
        if (s) {
            auto i1 = arr->data.begin() + std::min((size_t)(args[1].number()), s);
            auto i2 = arr->data.begin() + std::min((size_t)(args[2].number()), s);
            arr->data.erase(i1, i2);
        }
    } else {
        vm->error("expected "s + std::to_string(2) + " arguments");;
    }
    return TackValue::null();
}
tack_func(remove_value) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    std::erase(arr->data, args[1]);
    return TackValue::null();
}
tack_func(remove_if) {
    check_args(2);
    check_arg(0, array);
    auto* arr = args[0].array();
    auto func = args[1];
    if (func.is_function()) {
        std::erase_if(arr->data, [&](TackValue val) {
            return vm->call(func, 1, &val).get_truthy();
        });
    } else if (func.is_cfunction()) {
        auto* f = func.cfunction();
        std::erase_if(arr->data, [&](TackValue val) {
            return f(vm, 1, &val).get_truthy();
        });
    } else {
        vm->error("expected function or cfunction in remove_if()");
    }
    return TackValue::null();
}
tack_func(min) {
    if (nargs == 0) {
        return TackValue::null();
    } else if (nargs == 1) {
        check_arg(0, array);
        auto arr = args[0].array();
        if (!arr->data.size()) {
            return TackValue::null();
        }
        auto _0 = arr->data.at(0);
        check_val(_0, number);
        auto m = _0.number();
        for (auto i = 1u; i < arr->data.size(); i++) {
            auto _i = arr->data.at(i);
            check_val(_i, number);
            m = std::min(m, _i.number());
        }
        return TackValue::number(m);
    } else {
        check_arg(0, number);
        auto m = args[0].number();
        for (auto i = 1; i < nargs; i++) {
            check_arg(i, number);
            m = std::min(m, args[i].number());
        }
        return TackValue::number(m);
    }
}
tack_func(max) {
    if (nargs == 0) {
        return TackValue::null();
    } else if (nargs == 1) {
        check_arg(0, array);
        auto arr = args[0].array();
        if (!arr->data.size()) {
            return TackValue::null();
        }
        auto _0 = arr->data.at(0);
        check_val(_0, number);
        auto m = _0.number();
        for (auto i = 1u; i < arr->data.size(); i++) {
            auto _i = arr->data.at(i);
            check_val(_i, number);
            m = std::max(m, _i.number());
        }
        return TackValue::number(m);
    } else {
        check_arg(0, number);
        auto m = args[0].number();
        for (auto i = 1; i < nargs; i++) {
            check_arg(i, number);
            m = std::max(m, args[i].number());
        }
        return TackValue::number(m);
    }
}
tack_func(join) {
    check_args(2);

    auto retval = std::stringstream {};

    check_arg(0, array);
    check_arg(1, string);
    auto* arr = args[0].array();
    auto* str = args[1].string();

    if (arr->data.size()) {
        for (auto i = arr->data.begin() + 1; i != arr->data.end(); i++) {
            check_val(*i, string);
            retval << str->data << (*i).string();
        }
    }

    return TackValue::string(vm->alloc_string(retval.str()));
}
// tack_func(zip)
// tack_func(enumerate)
// tack_func(reverse)

// array and string
tack_func(slice) {
    check_args(3);
    check_arg(1, number);
    check_arg(2, number);
    auto l = (int)args[1].number();
    auto r = (int)args[2].number();

    if (args[0].is_array()) {
        auto* arr = args[0].array();
        auto s = (int)arr->data.size();
        if (l < 0 || l > s) {
            vm->error("slice index out of range");
        }
        if (r < 0 || r > s) {
            vm->error("slice index out of range");
        }
        auto ret = vm->alloc_array();
        ret->data.reserve(r - l);
        for (auto i = l; i < r; i++) {
            ret->data.emplace_back(arr->data.at(i));
        }
        return TackValue::array(ret);
    } else if (args[0].is_string()) {
        auto* str = args[0].string();
        auto s = (int)str->data.size();
        if (l < 0 || l > s) {
            vm->error("slice index out of range");
        }
        if (r < 0 || r > s) {
            vm->error("slice index out of range");
        }
        auto ret = vm->alloc_string(str->data.substr(l, r - l));
        return TackValue::string(ret);
    } else {
        vm->error("slice expected array or string");
    }
    return TackValue::null();
}
tack_func(find) {
    if (nargs < 2 || nargs > 3) {
        vm->error("find expected 2 or 3 arguments");
    }
    
    auto offset = 0;
    if (nargs == 3) {
        check_arg(2, number);
        offset = (int)args[2].number();
    }    
    
    if (args[0].is_array()) {
        auto* arr = args[0].array();
        auto val = args[1];
        for (auto i = 0u; i < arr->data.size(); i++) {
            if (arr->data.at(i) == val) {
                return TackValue::number(i);
            }
        }
        return TackValue::number(-1);
    } else if (args[0].is_string()) {
        auto* str = args[0].string();
        check_arg(1, string);
        auto* sub = args[1].string();
        auto f = str->data.find(sub->data, offset);
        if (f == std::string::npos) {
            return TackValue::number(-1);
        }
        return TackValue::number(f);
    } else {
        vm->error("find expected array or string");
    }
    return TackValue::null();
}

// string
tack_func(chr) {
    // integer to character
    check_args(1);
    check_arg(0, number);
    return TackValue::string(vm->alloc_string(std::string(1, (char)args[0].number())));
}
tack_func(ord) {
    // character to integer
    check_args(1);
    check_arg(0, string);
    auto str = args[0].string();
    auto ch = str->data.size() ? str->data[0] : 0.0;
    return TackValue::number((char)ch);
}
tack_func(tonumber) {
    // TODO: ignoring leading whitespace and trailing characters
    // eg "123.45foo" -> 123.45
    // is this desired behaviour
    check_args(1);
    check_arg(0, string);
    auto* str = args[0].string();
    auto* data = str->data.data();
    auto* end = (char*)nullptr;
    auto retval = std::strtod(data, &end);
    auto dist = end - data;
    if (dist != 0) {
        return TackValue::number(retval);
    }
    vm->error("could not convert to number");
    return TackValue::null();
}
tack_func(tolower) {
    check_args(1);
    check_arg(0, string);
    auto* str = vm->alloc_string(args[0].string()->data);
    for (auto& c : str->data) {
        c = std::tolower(c);
    }
    return TackValue::string(str);
}
tack_func(toupper) {
    check_args(1);
    check_arg(0, string);
    auto* str = vm->alloc_string(args[0].string()->data);
    for (auto& c : str->data) {
        c = std::toupper(c);
    }
    return TackValue::string(str);
}
tack_func(split) {
    // TODO: inefficient
    auto retval = std::vector<std::string> {};
    if (nargs == 1) {
        check_arg(0, string);
        retval = split_ws(args[0].string()->data);
    } else {
        check_arg(0, string);
        check_arg(1, string);
        if (nargs != 2) {
            vm->error("expected "s + std::to_string(2) + " arguments");;
        }
        auto* sep = args[1].string();
        if (!sep->data.size()) {
            vm->error("empty split() separator");
        }
        retval = split(args[0].string()->data, sep->data);
    }
    auto* arr = vm->alloc_array();
    for (auto& s : retval) {
        arr->data.emplace_back(TackValue::string(vm->alloc_string(s)));
    }
    return TackValue::array(arr);
}
// tack_func(strip)
// tack_func(format)
tack_func(replace) {
    check_args(3);
    check_arg(0, string);
    check_arg(1, string);
    check_arg(2, string);
    auto* str = args[0].string();
    auto* sub = args[1].string();
    auto* rep = args[2].string();
    return TackValue::string(vm->alloc_string(replace_all(str->data, sub->data, rep->data)));
}
tack_func(isupper) {
    check_args(1);
    check_arg(0, string);
    auto* str = args[0].string();
    for (auto c : str->data) {
        if (!std::isupper(c)) {
            return TackValue::false_();
        }
    }
    return TackValue::true_();
}
tack_func(islower) {
    check_args(1);
    check_arg(0, string);
    auto* str = args[0].string();
    for (auto c : str->data) {
        if (!std::islower(c)) {
            return TackValue::false_();
        }
    }
    return TackValue::true_();
}
tack_func(isdigit) {
    check_args(1);
    check_arg(0, string);
    auto* str = args[0].string();
    for (auto c : str->data) {
        if (!std::isdigit(c)) {
            return TackValue::false_();
        }
    }
    return TackValue::true_();
}
tack_func(isalnum) {
    check_args(1);
    check_arg(0, string);
    auto* str = args[0].string();
    for (auto c : str->data) {
        if (!std::isalnum(c)) {
            return TackValue::false_();
        }
    }
    return TackValue::true_();
}

// object
tack_func(keys) {
    check_args(1);
    check_arg(0, object);
    auto* obj = args[0].object();
    auto* arr = vm->alloc_array();
    for (auto i = obj->data.begin(); i != obj->data.end(); i = obj->data.next(i)) {
        auto& key = obj->data.key_at(i);
        auto cached = vm->intern_string(key);
        arr->data.push_back(TackValue::string(cached));
    }
    return TackValue::array(arr);
}
tack_func(values) {
    check_args(1);
    check_arg(0, object);
    auto* obj = args[0].object();
    auto* arr = vm->alloc_array();
    for (auto i = obj->data.begin(); i != obj->data.end(); i = obj->data.next(i)) {
        arr->data.push_back(obj->data.value_at(i));
    }
    return TackValue::array(arr);
}

// math
tack_func(radtodeg) {
    check_args(1);
    check_arg(0, number);
    return TackValue::number(args[0].number() * 180.f / pi);
}
tack_func(degtorad) {
    check_args(1);
    check_arg(0, number);
    return TackValue::number(args[0].number() * pi / 180.f);
}
tack_func(lerp) {
    check_args(3);
    check_arg(0, number);
    check_arg(1, number);
    check_arg(2, number);
    auto a = args[0].number();
    auto b = args[1].number();
    auto c = args[2].number();
    return TackValue::number(a + (b - a) * c);
}
tack_func(clamp) {
    check_args(3);
    check_arg(0, number);
    check_arg(1, number);
    check_arg(2, number);
    return TackValue::number(std::max(args[2].number(), std::min(args[1].number(), args[0].number())));
}
tack_func(saturate) {
    check_args(1);
    check_arg(0, number);
    return TackValue::number(std::max(0.0, std::min(1.0, args[0].number())));
}
// TODO: more interpolation: smoothstep/cubic, beziers, ...

void Interpreter::add_libs() {
    auto vm = this;
    // generic
    vm->set_global("print", TackValue::cfunction(tack_print), true);
    tack_bind(random);
    tack_bind(clock);
    tack_bind(gc_disable);
    tack_bind(gc_enable);
    // tack_bind(read_file);
    // tack_bind(write_file);
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

    // math
    tack_bind(radtodeg);
    tack_bind(degtorad);
    tack_bind(lerp);
    tack_bind(clamp);
    tack_bind(saturate);

    vm->set_global("pi", TackValue::number(pi), true);
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
}


#pragma GCC diagnostic pop
