#include "value.h"

#include <iostream>

std::ostream& operator<<(std::ostream& o, const mat4& v) { return o << "mat4 { ... }"; }
std::ostream& operator<<(std::ostream& o, const vec2& v) { return o << "vec2 { " << v.x << ", " << v.y << " }"; }
std::ostream& operator<<(std::ostream& o, const vec3& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << " }"; }
std::ostream& operator<<(std::ostream& o, const vec4& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << " }"; }
std::ostream& operator<<(std::ostream& o, const ArrayType& arr);
std::ostream& operator<<(std::ostream& o, ObjectType& obj);
std::ostream& operator<<(std::ostream& o, const Value& v) {
    if (!std::isnan(v._d)) {
        return o << value_to_number(v);
    }
    switch (v._i & type_bits) {
        case (uint64_t)Type::Null:      return o << "null";
        case (uint64_t)Type::Boolean:   return o << (value_to_boolean(v) ? "true" : "false");
        // case (uint64_t)Type::Integer:   return o << value_to_integer(v);
        case (uint64_t)Type::String:    return o << value_to_string(v);
        case (uint64_t)Type::Pointer:   return o << value_to_pointer(v);
        case (uint64_t)Type::Object:    return o << *value_to_object(v);
        case (uint64_t)Type::Array:     return o << *value_to_array(v);
        case (uint64_t)Type::Mat4:      return o << *value_to_mat4(v);
        case (uint64_t)Type::Vec2:      return o << *value_to_vec2(v);
        case (uint64_t)Type::Vec3:      return o << *value_to_vec3(v);
        case (uint64_t)Type::Vec4:      return o << *value_to_vec4(v);
        case (uint64_t)Type::Function:  return o << "function: " << std::hex << v._p;
        case (uint64_t)Type::CFunction: return o << "c-func:   " << std::hex << v._p;
        case type_bits_boxed:           return o << "box:      " << std::hex << v._p << "(" << std::hex << value_to_boxed(v)->value._p << ")";
        default:                        return o << "(unknown) " << std::hex << v._i;
    }
}
std::ostream& operator<<(std::ostream& o, const ArrayType& arr) {
    o << "array [";
    if (arr.values.size()) {
        o << ' ' << arr.values[0];
        for (auto i = 1; i < arr.values.size(); i++) {
            o << ", " << arr.values[i];
        }
        o << ' ';
    }
    return o << ']';
}
std::ostream& operator<<(std::ostream& o, ObjectType& obj) {
    o << "object {";
    if (obj.length()) {
        auto i = obj.begin();
        o << ' ' << obj.key(i) << " = " << obj.value(i);
        i = obj.next(i);
        for (auto e = obj.end(); i != e; i = obj.next(i)) {
            o << ", " << obj.key(i) << " = " << obj.value(i);
        }
        o << ' ';
    }

    // HACK: Not using the first/next methods here, reserve those
    // this is the only other site that iterates ObjectType; 
    return o << '}';
}



