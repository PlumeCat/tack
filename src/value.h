#pragma once

#include <cmath>
#include <jlib/hash_map.h>

#pragma region Config
#define Straw_32Bit             0 // 1 for 32 bit pointers, 0 for 64 bit pointers
#define Straw_CheckPointerBits  0
#define straw_error(err)        throw runtime_error(err); // what to do when there's an error
#pragma endregion
static constexpr uint64_t operator ""_u64(unsigned long long t) { return t; }
#define cat(a, b) a##b
#define declare_type_bits(N, V) static const auto cat(type_bits_, N) = cat(V, _u64) << 48;

static constexpr auto nan_bits = (0x7ff_u64 << 52);
static constexpr auto type_bits = (0xf_u64 << 48);
static constexpr auto boolean_bits = 0x1_u64;
static constexpr auto integer_bits = 0x00000000ffffffff_u64;
#if (Straw_32Bit == 0)
static constexpr auto pointer_bits = 0x0000ffffffffffff_u64;
#else
static constexpr auto pointer_bits = 0x00000000ffffffff_u64;
#endif


//declare_type_bits(number, ...)  // not needed, it's implicit with the nan bits
static const auto type_bits_number = nan_bits;
declare_type_bits(null,   0b1111) // UINT64_MAX is null
declare_type_bits(pointer,0b0000) // unmanaged user pointer - pass by value
declare_type_bits(boolean,0b0001)
declare_type_bits(integer,0b0011)
declare_type_bits(______1,0b0010) // perhaps byte4 aka Color for 32bit colors?

// Algebraics - they have special heaps of their own
declare_type_bits(vec2,   0b0100)
declare_type_bits(vec3,   0b0101)
declare_type_bits(vec4,   0b0110)
declare_type_bits(mat4,   0b0111)

// Other
declare_type_bits(boxed,  0b1000)
declare_type_bits(string, 0b1001) // TODO: decide about interning/hashing/SSO
declare_type_bits(array,  0b1011)
declare_type_bits(object, 0b1010)

// Unused
declare_type_bits(function,0b1100)
declare_type_bits(______3,0b1101)
declare_type_bits(______4,0b1110)

#undef declare_type_bits


// types from users perspective
enum class Type : uint64_t {
    // scalars
    Null = type_bits_null,
    Boolean = type_bits_boolean,
    Integer = type_bits_integer,
    Number = type_bits_number,
    Pointer = type_bits_pointer,

    // algebraics
    String = type_bits_string,
    Vec2 = type_bits_vec2,
    Vec3 = type_bits_vec3,
    Vec4 = type_bits_vec4,
    Mat4 = type_bits_mat4,

    // containers
    Array = type_bits_array,
    Object = type_bits_object,

    // function or closure
    Function = type_bits_function,

    /*
    other possibilities:
        C_function
        color (byte4 aka uint32)
        tuple
        set
    */
};

union Value; // forward declare for who needs it
using BoxType = Value;
using NullType = nullptr_t;
using BooleanType = bool;
using IntegerType = int;
using NumberType = double;
using StringType = string;
using ObjectType = hash_map<string, Value>;
using ArrayType = vector<Value>;
struct FunctionType {
    vector<Value> captures;
    uint32_t code;
};
struct vec2 { float x, y; };
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };
struct mat4 { float m[16]; };

union Value {
    uint64_t i;
    double d;
    void* p;
};

/*
internal type representation using nan-tagging

first three hex digits are first 12 bits of our double
    sign bit and 11 exponent bits
they must all be 1 to indicate nan
fourth hex digit is the type, so we can have up to 17 types (including number which is indicated by the nan bits)
so we can use bits 12-15 to indicate type

Algebraics:
    store the index in the lower 32 bits of Value
    they will be garbage collected as normal
    (TODO: decide if stack allocated)

"Boxed"
    boxed type refers to any scalar type that would normally live on the stack, be passed by value and die at scope exit
    but is captured by a closure so needs to be boxed
    this is the only circumstance in which they are created, otherwise the user won't 


*/

// create value
constexpr Value value_null()      { return { UINT64_MAX }; }
constexpr Value value_false()     { return { nan_bits | type_bits_boolean }; }
constexpr Value value_true()      { return { nan_bits | type_bits_boolean | 1 }; }

// create value
Value value_from_boolean(bool b)     { return { nan_bits | type_bits_boolean | b }; }
Value value_from_integer(int32_t i)   { return { nan_bits | type_bits_integer | i }; }
Value value_from_number(double d) { return { .d = d }; /* TODO: check for nan and mask off! */}
Value value_from_pointer(void* ptr) {
    // convert the pointer
    // for 64 bit, only 48 bits should be used, so mask them off
    // (there's the option to check whether higher bits were used)
    uint64_t u_ptr = (uint64_t)ptr;

#if (Straw_CheckPointerBits == 0)
    if (u_ptr & (~pointer_bits)) {
        straw_error("received a pointer with upper 12 bits in use")
    }
#endif

    // should also be valid for 32 bit pointers
    return { nan_bits | type_bits_pointer | u_ptr };
}
Value value_from_boxed(BoxType* box)    { return { nan_bits | type_bits_boxed  | uint64_t(box) }; }
Value value_from_string(StringType* str){ return { nan_bits | type_bits_string | uint64_t(str) }; }
Value value_from_object(ObjectType* obj){ return { nan_bits | type_bits_object | uint64_t(obj) }; }
Value value_from_array(ArrayType* arr)  { return { nan_bits | type_bits_array  | uint64_t(arr) }; }
Value value_from_vec2(vec2* v2)  { return { nan_bits | type_bits_vec2   | uint64_t(v2) }; }
Value value_from_vec3(vec3* v3)  { return { nan_bits | type_bits_vec3   | uint64_t(v3) }; }
Value value_from_vec4(vec4* v4)  { return { nan_bits | type_bits_vec4   | uint64_t(v4) }; }
Value value_from_mat4(mat4* m4)  { return { nan_bits | type_bits_mat4   | uint64_t(m4) }; }

// type checks
Type value_get_type(Value v)   { return isnan(v.d) ? (Type)(v.i & type_bits) : Type::Number; }
bool value_is_null(Value v)    { return v.i == UINT64_MAX; }
bool value_is_boolean(Value v) { return (v.i & type_bits) == type_bits_boolean; }
bool value_is_integer(Value v) { return (v.i & type_bits) == type_bits_integer; }
bool value_is_number(Value v)  { return !isnan(v.d); }
bool value_is_pointer(Value v) { return (v.i & type_bits) == type_bits_pointer; }
bool value_is_boxed(Value v)   { return (v.i & type_bits) == type_bits_boxed;   }
bool value_is_string(Value v)  { return (v.i & type_bits) == type_bits_string;  }
bool value_is_object(Value v)  { return (v.i & type_bits) == type_bits_object;  }
bool value_is_array(Value v)   { return (v.i & type_bits) == type_bits_array;   }
bool value_is_vec2(Value v)    { return (v.i & type_bits) == type_bits_vec2; }
bool value_is_vec3(Value v)    { return (v.i & type_bits) == type_bits_vec3; }
bool value_is_vec4(Value v)    { return (v.i & type_bits) == type_bits_vec4; }
bool value_is_mat4(Value v)    { return (v.i & type_bits) == type_bits_mat4; }

StringType* value_to_string(Value v){ return (StringType*)(v.i & pointer_bits); }
ArrayType* value_to_array(Value v)  { return (ArrayType*)(v.i & pointer_bits); }
ObjectType* value_to_object(Value v){ return (ObjectType*)(v.i & pointer_bits); }
BoxType* value_to_box(Value v)      { return (BoxType*)(v.i & pointer_bits); }
vec2* value_to_vec2(Value v)        { return (vec2*)(v.i & pointer_bits); }
vec3* value_to_vec3(Value v)        { return (vec3*)(v.i & pointer_bits); }
vec4* value_to_vec4(Value v)        { return (vec4*)(v.i & pointer_bits); }
mat4* value_to_mat4(Value v)        { return (mat4*)(v.i & pointer_bits); }
uint64_t value_to_null(Value v)     { return v.i; } // ???
bool value_to_boolean(Value v)      { return (bool)(v.i & boolean_bits); }
int32_t value_to_integer(Value v)   { return (int32_t)(v.i & integer_bits); }
double value_to_number(Value v)     { return v.d; } // ???
void* value_to_pointer(Value v)     { return (void*)(v.i & pointer_bits); }

ostream& operator<<(ostream& o, const mat4& v) { return o << "mat4 { ... }"; }
ostream& operator<<(ostream& o, const vec2& v) { return o << "vec2 { " << v.x << ", " << v.y << " }"; }
ostream& operator<<(ostream& o, const vec3& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << " }"; }
ostream& operator<<(ostream& o, const vec4& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << " }"; }
ostream& operator<<(ostream& o, const Value& v) {
    switch (value_get_type(v)) {
        case Type::Null:        return o << "null";
        case Type::Boolean:     return o << value_to_boolean(v);
        case Type::Integer:     return o << value_to_integer(v);
        case Type::Number:      return o << value_to_number(v);
        case Type::String:      return o << *value_to_string(v);
        case Type::Pointer:     return o << value_to_pointer(v);
        case Type::Object:      return o << "object { ... }"; //*value_to_object(v);
        case Type::Array:       return o << "array [" << value_to_array(v)->size() << "]"; //*value_to_array(v);
        case Type::Function:    return o << "fn() { ... }";
        case Type::Mat4:        return o << *value_to_mat4(v);
        case Type::Vec2:        return o << *value_to_vec2(v);
        case Type::Vec3:        return o << *value_to_vec3(v);
        case Type::Vec4:        return o << *value_to_vec4(v);
    }
}
