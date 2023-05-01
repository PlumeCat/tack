#include "value.h"

#include <iostream>
#include <iomanip>

std::ostream& operator<<(std::ostream& o, ArrayType* arr);
std::ostream& operator<<(std::ostream& o, ObjectType* obj);

std::ostream& operator<<(std::ostream& o, const Value& v) {
    if (!std::isnan(v._d)) {
        if (v._d == trunc(v._d)) {
            return o << (uint64_t)v._d;
        }
        return o << v._d;
    }
    switch (v._i & type_bits) {
        case (uint64_t)Type::Null:      return o << "null";
        case (uint64_t)Type::Boolean:   return o << (value_to_boolean(v) ? "true" : "false");
        case (uint64_t)Type::String:    return o << value_to_string(v)->data;
        case (uint64_t)Type::Pointer:   return o << value_to_pointer(v);
        case (uint64_t)Type::Object:    return o << value_to_object(v);
        case (uint64_t)Type::Array:     return o << value_to_array(v);
        case (uint64_t)Type::Function:  return o << "function: " << std::hex << v._p;
        case (uint64_t)Type::CFunction: return o << "c-func:   " << std::hex << v._p;
        case type_bits_boxed:           return o << "box:      " << std::hex << v._p << "(" << std::hex << value_to_boxed(v)->value._p << ")";
        default:                        return o << "(unknown) " << std::hex << v._i;
    }
}
bool value_get_truthy(Value v) {
    return
        (!std::isnan(v._d))                         ? (v._d != 0.0) : // number -> compare to 0
        (v._i == UINT64_MAX)                        ? false :
        ((v._i & type_bits) == type_bits_boolean)   ? (v._i & boolean_bits) :
        true;
}
std::ostream& operator<<(std::ostream& o, ArrayType* arr) {
    o << "array [";
    if (arr->size()) {
        o << ' ' << (*arr)[0];
        for (auto i = 1u; i < arr->size(); i++) {
            o << ", " << (*arr)[i];
        }
        o << ' ';
    }
    return o << ']';
}
std::ostream& operator<<(std::ostream& o, ObjectType* obj) {
    o << "object {";
    if (obj->size()) {
        auto i = obj->begin();
        o << ' ' << obj->key_at(i) << " = " << obj->value_at(i);
        // i = obj->next(i);
        i = obj->next(i);
        for (auto e = obj->end(); i != e; i = obj->next(i)) {
            o << ", " << obj->key_at(i) << " = " << obj->value_at(i);
        }
        o << ' ';
    }

    // HACK: Not using the first/next methods here, reserve those
    // this is the only other site that iterates ObjectType; 
    return o << '}';
}



