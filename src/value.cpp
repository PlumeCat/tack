#include "value.h"

#include <iostream>
#include <sstream>
#include <iomanip>

bool value_get_truthy(Value v) {
    return
        (!std::isnan(v._d))                         ? (v._d != 0.0) : // number -> compare to 0
        (v._i == UINT64_MAX)                        ? false :
        ((v._i & type_bits) == type_bits_boolean)   ? (v._i & boolean_bits) :
        true;
}
std::string value_get_string(Value v) {
    auto s = std::stringstream {};
    if (!std::isnan(v._d)) {
        if (v._d == trunc(v._d)) {
            s << (int64_t)v._d;
        } else {
            s << v._d;
        }
        return s.str();
    }
    switch (v._i & type_bits) {
        case (uint64_t)Type::Null:      { s << "null"; break; }
        case (uint64_t)Type::Boolean:   { s << (value_to_boolean(v) ? "true" : "false"); break; }
        case (uint64_t)Type::String:    { s << value_to_string(v)->data; break; }
        case (uint64_t)Type::Pointer:   { s << value_to_pointer(v); break; }
        case (uint64_t)Type::Function:  { s << "function: " << std::hex << v._p; break; }
        case (uint64_t)Type::CFunction: { s << "c-func:   " << std::hex << v._p; break; }
        case type_bits_boxed:           { s << "box:      " << std::hex << v._p << "(" << std::hex << value_to_boxed(v)->value._p << ")"; break; }
        case (uint64_t)Type::Object: { 
            auto* obj = value_to_object(v);
            s << "object {";
            if (obj->size()) {
                auto i = obj->begin();
                s << ' ' << obj->key_at(i) << " = " << value_get_string(obj->value_at(i));
                // i = obj->next(i);
                i = obj->next(i);
                for (auto e = obj->end(); i != e; i = obj->next(i)) {
                    s << ", " << obj->key_at(i) << " = " << value_get_string(obj->value_at(i));
                }
                s << ' ';
            }

            s << '}';
            break;
        }
        case (uint64_t)Type::Array: {
            auto* arr = value_to_array(v);
            s << "array [";
            if (arr->size()) {
                s << " " << value_get_string(arr->at(0));
                for (auto i = 1u; i < arr->size(); i++) {
                    s << ", " << value_get_string(arr->at(i));
                }
                s << ' ';
            }
            s << ']';
            break;
        }
        default: { s << "(unknown) " << std::hex << v._i; break; }
    }
    return s.str();
}



