#include "value.h"

#include <iostream>

std::ostream& operator<<(std::ostream& o, const mat4& v) { return o << "mat4 { ... }"; }
std::ostream& operator<<(std::ostream& o, const vec2& v) { return o << "vec2 { " << v.x << ", " << v.y << " }"; }
std::ostream& operator<<(std::ostream& o, const vec3& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << " }"; }
std::ostream& operator<<(std::ostream& o, const vec4& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << " }"; }
std::ostream& operator<<(std::ostream& o, const ArrayType& arr);
std::ostream& operator<<(std::ostream& o, const ObjectType& obj);
std::ostream& operator<<(std::ostream& o, const Value& v) {
    if (!std::isnan(v._d)) {
        return o << value_to_number(v);
    }
    switch (v._i & type_bits) {
        case (uint64_t)Type::Null:      return o << "null";
        case (uint64_t)Type::Boolean:   return o << (value_to_boolean(v) ? "true" : "false");
        case (uint64_t)Type::Integer:   return o << value_to_integer(v);
        case (uint64_t)Type::String:    return o << *value_to_string(v);
        case (uint64_t)Type::Pointer:   return o << value_to_pointer(v);
        case (uint64_t)Type::Object:    return o << "object {" << *value_to_object(v) << "}";
        case (uint64_t)Type::Array:     return o << "array [" << *value_to_array(v) << "]";
        case (uint64_t)Type::Mat4:      return o << *value_to_mat4(v);
        case (uint64_t)Type::Vec2:      return o << *value_to_vec2(v);
        case (uint64_t)Type::Vec3:      return o << *value_to_vec3(v);
        case (uint64_t)Type::Vec4:      return o << *value_to_vec4(v);
        case (uint64_t)Type::Closure:   return o << "closure: " << std::hex << v._p;
        case type_bits_boxed:           return o << "box: " << std::hex << v._p << "(" << std::hex << value_to_boxed(v)->_p << ")";
        default:                        return o << "(unknown)" << std::hex << v._i;
    }
}
std::ostream& operator<<(std::ostream& o, const ArrayType& arr) {
    if (arr.size()) {
        o << ' ' << arr[0];
        for (auto i = 1; i < arr.size(); i++) {
            o << ", " << arr[i];
        }
        o << ' ';
    }
    return o;
}
std::ostream& operator<<(std::ostream& o, const ObjectType& obj) {
    if (obj.size()) {
        auto i = obj.begin();
        o << ' ' << i->first << " = " << i->second;
        for (auto e = (i++, obj.end()); i != e; i++) {
            o << ", " << i->first << " = " << i->second;
        }
        o << ' ';
    }
    return o;
}
