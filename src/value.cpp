#include "interpreter.h"
#include "compiler.h"

#include <iostream>
#include <sstream>
#include <iomanip>

bool Value::get_truthy() {
    return
        (!std::isnan(_d))                             ? (_d != 0.0) : // number -> compare to 0
        (_i == UINT64_MAX)                            ? false :
        ((_i & type_bits) == (uint64_t)Type::Boolean) ? (_i & boolean_bits) :
        true;
}
std::string Value::get_string() {
    auto s = std::stringstream {};
    if (!std::isnan(_d)) {
        if (_d == trunc(_d)) {
            s << (int64_t)_d;
        } else {
            s << _d;
        }
        return s.str();
    }
    switch (_i & type_bits) {
        case (uint64_t)Type::Null:      { s << "null"; break; }
        case (uint64_t)Type::Boolean:   { s << (boolean() ? "true" : "false"); break; }
        case (uint64_t)Type::String:    { s << string()->data; break; }
        case (uint64_t)Type::Pointer:   { s << pointer(); break; }
        case (uint64_t)Type::Function:  { s << "function: " << function()->bytecode->name; break; }
        case (uint64_t)Type::CFunction: { s << "c-func:   " << std::hex << _p; break; }
        case type_bits_boxed:           { s << "box:      " << std::hex << _p << "(" << std::hex << value_to_boxed(*this)->value._p << ")"; break; }
        case (uint64_t)Type::Object: { 
            auto* obj = object();
            s << "object {";
            if (obj->data.size()) {
                auto i = obj->data.begin();
                s << ' ' << obj->data.key_at(i) << " = " << obj->data.value_at(i).get_string();
                // i = obj->data.next(i);
                i = obj->data.next(i);
                for (auto e = obj->data.end(); i != e; i = obj->data.next(i)) {
                    s << ", " << obj->data.key_at(i) << " = " << obj->data.value_at(i).get_string();
                }
                s << ' ';
            }

            s << '}';
            break;
        }
        case (uint64_t)Type::Array: {
            auto* arr = array();
            s << "array [";
            if (arr->data.size()) {
                s << " " << arr->data.at(0).get_string();
                for (auto i = 1u; i < arr->data.size(); i++) {
                    s << ", " << arr->data.at(i).get_string();
                }
                s << ' ';
            }
            s << ']';
            break;
        }
        default: { s << "(unknown) " << std::hex << _i; break; }
    }
    return s.str();
}



