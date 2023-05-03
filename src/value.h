#pragma once

#include <cmath>
#include <vector>
#include <iostream>

#include "khash2.h"

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
declare_type_bits(______1,  0b0010) // 0x2
declare_type_bits(______2,  0b0011) // 0x3
// Unused currently
declare_type_bits(______3,  0b0100) // 0x4
declare_type_bits(______4,  0b0101) // 0x5
declare_type_bits(______5,  0b0110) // 0x6
declare_type_bits(______6,  0b0111) // 0x7
// Native
declare_type_bits(string,   0b1000) // 0x8
declare_type_bits(object,   0b1001) // 0x9
declare_type_bits(boxed,    0b1011) // 0xb
declare_type_bits(array,    0b1010) // 0xa
// Other
declare_type_bits(function, 0b1100) // 0xc
declare_type_bits(cfunction,0b1101) // 0xd
declare_type_bits(cobject,  0b1110) // 0xe
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

    // containers
    Array = type_bits_array,
    Object = type_bits_object,
    CObject = type_bits_cobject,
    Function = type_bits_function,
    CFunction = type_bits_cfunction,

    /*
    other possibilities:
        color (byte4 aka uint32)
        "class" - some kind of special class object with copydown inheritance
    */
};

struct CodeFragment;
struct Interpreter;

// TODO: this is straight undefined behaviour
struct Value {
    union {
        uint64_t _i;
        double _d;
        void* _p;
    };
};

#define GCType() bool marker = false; uint32_t refcount = 0;
struct GCType {
    bool marker = false;
    uint32_t refcount = 0; // for retaining in host
};

using NullType = nullptr_t;
using CFunctionType = Value(*)(Interpreter*, int, Value*);

struct BoxType {
    Value value;
    GCType()
};
struct StringType {
    std::string data;
    GCType()
};
struct ArrayType : std::vector<Value> {
    GCType()
};
struct ObjectType : KHash<std::string, Value> {
    GCType()
};
struct CObjectType {
    void* data;
    GCType()
};
struct FunctionType {
    CodeFragment* bytecode;
    std::vector<Value> captures; // contains boxes
    GCType()
};


// create value
inline constexpr Value value_null()                     { return { UINT64_MAX }; }
inline constexpr Value value_false()                    { return { nan_bits | type_bits_boolean }; }
inline constexpr Value value_true()                     { return { nan_bits | type_bits_boolean | 1 }; }

// create value
inline Value value_from_boolean(bool b)                 { return { nan_bits | type_bits_boolean | (uint64_t)b }; }
inline Value value_from_number(double d)                { return { ._d = d }; /* TODO: check for nan and mask off? */}
inline Value value_from_pointer(void* ptr)              { return { nan_bits | type_bits_pointer | ((uint64_t)ptr & pointer_bits) }; }
inline Value value_from_function(FunctionType* func)    { return { nan_bits | type_bits_function | uint64_t(func) }; }
inline Value value_from_boxed(BoxType* box)             { return { nan_bits | type_bits_boxed  | uint64_t(box) }; }
inline Value value_from_string(StringType* str)         { return { nan_bits | type_bits_string | uint64_t(str) }; }
inline Value value_from_object(ObjectType* obj)         { return { nan_bits | type_bits_object | uint64_t(obj) }; }
inline Value value_from_array(ArrayType* arr)           { return { nan_bits | type_bits_array  | uint64_t(arr) }; }
inline Value value_from_cfunction(CFunctionType cfunc)  { return { nan_bits | type_bits_cfunction | uint64_t(cfunc) }; }
inline Value value_from_cobject(CObjectType* obj)       { return { nan_bits | type_bits_cobject | uint64_t(obj) }; }
// type checks

/*
0, null, false are falsy
everything else is truthy, including empty string, empty object, null pointer, etc
*/
bool value_get_truthy(Value v);
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
inline bool value_is_cobject(Value v)                   { return (v._i & type_bits) == type_bits_cobject; }

#define check(ty) if (!value_is_##ty(v)) throw std::runtime_error("type error: expected " #ty);
inline StringType* value_to_string(Value v)             { check(string);    return (StringType*)(v._i & pointer_bits); }
inline ArrayType* value_to_array(Value v)               { check(array);     return (ArrayType*)(v._i & pointer_bits); }
inline ObjectType* value_to_object(Value v)             { check(object);    return (ObjectType*)(v._i & pointer_bits); }
inline BoxType* value_to_boxed(Value v)                 { check(boxed);     return (BoxType*)(v._i & pointer_bits); }
inline FunctionType* value_to_function(Value v)         { check(function);  return (FunctionType*)(v._i & pointer_bits); }
inline CFunctionType value_to_cfunction(Value v)        { check(cfunction); return (CFunctionType)(v._i & pointer_bits); }
inline uint64_t value_to_null(Value v)                  { check(null);      return v._i; } // ???
inline bool value_to_boolean(Value v)                   { check(boolean);   return (bool)(v._i & boolean_bits); }
inline double value_to_number(Value v)                  { check(number);    return v._d; } // ???
inline void* value_to_pointer(Value v)                  { check(pointer);   return (void*)(v._i & pointer_bits); }
inline CObjectType* value_to_cobject(Value v)           { check(cobject);   return (CObjectType*)(v._i & pointer_bits); }

std::ostream& operator<<(std::ostream& o, const Value& v);
