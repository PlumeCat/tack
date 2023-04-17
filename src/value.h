#pragma once

#include <cmath>
#include <vector>
#include <iostream>

static constexpr uint64_t operator ""_u64(unsigned long long t) { return t; }
static constexpr auto nan_bits = (0x7ff0_u64 << 48);
static constexpr auto type_bits = (0xf_u64 << 48);
static constexpr auto boolean_bits = 0x1_u64;
static constexpr auto integer_bits = 0x00000000ffffffff_u64;
static constexpr auto pointer_bits = 0x0000ffffffffffff_u64;

#define cat(a, b) a##b
#define declare_type_bits(N, V) static const auto cat(type_bits_, N) = cat(V, _u64) << 48;
//declare_type_bits(number, ...)  // not needed, it's implicit with the nan bits
static const auto type_bits_number = nan_bits;
declare_type_bits(pointer,  0b0000) // 0x0 // unmanaged user pointer - pass by value
declare_type_bits(boolean,  0b0001) // 0x1
declare_type_bits(______2,  0b0011) // 0x3
declare_type_bits(______1,  0b0010) // 0x2
// "Algebraic"
declare_type_bits(vec2,     0b0100) // 0x4
declare_type_bits(vec3,     0b0101) // 0x5
declare_type_bits(vec4,     0b0110) // 0x6
declare_type_bits(mat4,     0b0111) // 0x7
// Other
declare_type_bits(string,   0b1000) // 0x8
declare_type_bits(object,   0b1001) // 0x9 // TODO: decide about interning/hashing/SSO
declare_type_bits(boxed,    0b1011) // 0xb // TODO: box won't be needed once we have labelled registers
declare_type_bits(array,    0b1010) // 0xa
// Unused
declare_type_bits(function, 0b1100) // 0xc
declare_type_bits(cfunction,0b1101) // 0xd
declare_type_bits(______4,  0b1110) // 0xe
declare_type_bits(null,     0b1111) // 0xf // UINT64_MAX is null
#undef declare_type_bits


// types from users perspective
enum class Type : uint64_t {
    // scalars
    Null = type_bits_null,
    Boolean = type_bits_boolean,
    Number = type_bits_number,
    String = type_bits_string,
    Pointer = type_bits_pointer,

    // algebraics
    Vec2 = type_bits_vec2,
    Vec3 = type_bits_vec3,
    Vec4 = type_bits_vec4,
    Mat4 = type_bits_mat4,

    // containers
    Array = type_bits_array,
    Object = type_bits_object,
    Function = type_bits_function,
    CFunction = type_bits_cfunction,

    /*
    other possibilities:
        C_function - might need its own type?
        int32 or uint32 - removed it once already
        color (byte4 aka uint32)
        "class" - some kind of special class object with copydown inheritance
    */
};

struct CodeFragment;

// TODO: this is straight undefined behaviour
struct Value {
    union {
        uint64_t _i;
        double _d;
        void* _p;
    };
};
struct GCType {
    bool marker = false;
    uint32_t refcount = 0; // for retaining in host
};

using NullType = nullptr_t;
using StringType = const char;

#include "object_type.h"

struct BoxType : GCType {
    Value value;
};
struct ArrayType : GCType {
    std::vector<Value> values;
};
struct FunctionType : GCType {
    CodeFragment* bytecode;
    std::vector<Value> captures; // contains boxes
};

using CFunctionType = Value(int, Value*);

struct vec2 { float x, y; };
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };
struct mat4 { float m[16]; };


// create value
inline constexpr Value value_null()                     { return { UINT64_MAX }; }
inline constexpr Value value_false()                    { return { nan_bits | type_bits_boolean }; }
inline constexpr Value value_true()                     { return { nan_bits | type_bits_boolean | 1 }; }

// create value
inline Value value_from_boolean(bool b)                 { return { nan_bits | type_bits_boolean | b }; }
inline Value value_from_number(double d)                { return { ._d = d }; /* TODO: check for nan and mask off? */}
inline Value value_from_pointer(void* ptr)              { return { nan_bits | type_bits_pointer | ((uint64_t)ptr & pointer_bits) }; }
inline Value value_from_function(FunctionType* func)    { return { nan_bits | type_bits_function | uint64_t(func) }; }
inline Value value_from_boxed(BoxType* box)             { return { nan_bits | type_bits_boxed  | uint64_t(box) }; }
inline Value value_from_string(const StringType* str)   { return { nan_bits | type_bits_string | uint64_t(str) }; }
inline Value value_from_object(ObjectType* obj)         { return { nan_bits | type_bits_object | uint64_t(obj) }; }
inline Value value_from_array(ArrayType* arr)           { return { nan_bits | type_bits_array  | uint64_t(arr) }; }
inline Value value_from_cfunction(CFunctionType* cfunc) { return { nan_bits | type_bits_cfunction | uint64_t(cfunc) }; }
inline Value value_from_vec2(vec2* v2)                  { return { nan_bits | type_bits_vec2   | uint64_t(v2) }; }
inline Value value_from_vec3(vec3* v3)                  { return { nan_bits | type_bits_vec3   | uint64_t(v3) }; }
inline Value value_from_vec4(vec4* v4)                  { return { nan_bits | type_bits_vec4   | uint64_t(v4) }; }
inline Value value_from_mat4(mat4* m4)                  { return { nan_bits | type_bits_mat4   | uint64_t(m4) }; }

// type checks
inline Type value_get_type(Value v)                     { return std::isnan(v._d) ? (Type)(v._i & type_bits) : Type::Number; }
inline bool value_is_null(Value v)                      { return v._i == UINT64_MAX; }
inline bool value_is_boolean(Value v)                   { return (v._i & type_bits) == type_bits_boolean; }
inline bool value_is_number(Value v)                    { return !std::isnan(v._d); }
inline bool value_is_pointer(Value v)                   { return (v._i & type_bits) == type_bits_pointer; }
inline bool value_is_boxed(Value v)                     { return std::isnan(v._d) && (v._i & type_bits) == type_bits_boxed;   }
inline bool value_is_function(Value v)                  { return (v._i & type_bits) == type_bits_function; }
inline bool value_is_cfunction(Value v)                 { return (v._i & type_bits) == type_bits_cfunction; }
inline bool value_is_string(Value v)                    { return (v._i & type_bits) == type_bits_string;  }
inline bool value_is_object(Value v)                    { return (v._i & type_bits) == type_bits_object;  }
inline bool value_is_array(Value v)                     { return (v._i & type_bits) == type_bits_array;   }
inline bool value_is_vec2(Value v)                      { return (v._i & type_bits) == type_bits_vec2; }
inline bool value_is_vec3(Value v)                      { return (v._i & type_bits) == type_bits_vec3; }
inline bool value_is_vec4(Value v)                      { return (v._i & type_bits) == type_bits_vec4; }
inline bool value_is_mat4(Value v)                      { return (v._i & type_bits) == type_bits_mat4; }

static void _check(bool c, char* msg) {
    if (!c) {
        throw std::runtime_error(std::string("type error: expected ") + msg);
    }
}
#define check(ty) _check(value_is_##ty(v), #ty)
// #define check(ty) if (!value_is_##ty(v)) throw std::runtime_error("type error: expected " #ty);
inline StringType* value_to_string(Value v)             { check(string);    return (StringType*)(v._i & pointer_bits); }
inline ArrayType* value_to_array(Value v)               { check(array);     return (ArrayType*)(v._i & pointer_bits); }
inline ObjectType* value_to_object(Value v)             { check(object);    return (ObjectType*)(v._i & pointer_bits); }
inline BoxType* value_to_boxed(Value v)                 { check(boxed);     return (BoxType*)(v._i & pointer_bits); }
inline FunctionType* value_to_function(Value v)         { check(function);  return (FunctionType*)(v._i & pointer_bits); }
inline CFunctionType* value_to_cfunction(Value v)       { check(cfunction); return (CFunctionType*)(v._i & pointer_bits); }
inline vec2* value_to_vec2(Value v)                     { check(vec2);      return (vec2*)(v._i & pointer_bits); }
inline vec3* value_to_vec3(Value v)                     { check(vec3);      return (vec3*)(v._i & pointer_bits); }
inline vec4* value_to_vec4(Value v)                     { check(vec4);      return (vec4*)(v._i & pointer_bits); }
inline mat4* value_to_mat4(Value v)                     { check(mat4);      return (mat4*)(v._i & pointer_bits); }
inline uint64_t value_to_null(Value v)                  { check(null);      return v._i; } // ???
inline bool value_to_boolean(Value v)                   { check(boolean);   return (bool)(v._i & boolean_bits); }
inline double value_to_number(Value v)                  { check(number);    return v._d; } // ???
inline void* value_to_pointer(Value v)                  { check(pointer);   return (void*)(v._i & pointer_bits); }
#undef check

std::ostream& operator<<(std::ostream& o, const Value& v);
