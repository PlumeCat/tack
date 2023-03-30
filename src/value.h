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
declare_type_bits(null,   0b1111) // 0xf // UINT64_MAX is null
declare_type_bits(pointer,0b0000) // 0x0 // unmanaged user pointer - pass by value
declare_type_bits(boolean,0b0001) // 0x1
declare_type_bits(integer,0b0011) // 0x3
declare_type_bits(______1,0b0010) // 0x2 // perhaps byte4 aka Color for 32bit colors?

// Algebraics - they have special heaps of their own
declare_type_bits(vec2,   0b0100) // 0x4
declare_type_bits(vec3,   0b0101) // 0x5
declare_type_bits(vec4,   0b0110) // 0x6
declare_type_bits(mat4,   0b0111) // 0x7

// Other
declare_type_bits(string, 0b1000) // 0x8
declare_type_bits(object, 0b1001) // 0x9 // TODO: decide about interning/hashing/SSO
declare_type_bits(boxed,  0b1011) // 0xb
declare_type_bits(array,  0b1010) // 0xa

// Unused
declare_type_bits(function,0b1100)// 0xc
declare_type_bits(closure, 0b1101)// 0xd
declare_type_bits(______4,0b1110) // 0xe

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
    // Boxed

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

struct Value; // forward declare for who needs it
using BoxType = Value;
using NullType = nullptr_t;
// using BooleanType = bool;
// using IntegerType = int;
// using NumberType = double;
using StringType = string;
using ObjectType = hash_map<string, Value>;
using ArrayType = vector<Value>;
using FunctionType = struct {
    uint32_t code;
};
using ClosureType = struct {
    FunctionType* func;
    vector<Value> captures; // contains boxes
};
struct vec2 { float x, y; };
struct vec3 { float x, y, z; };
struct vec4 { float x, y, z, w; };
struct mat4 { float m[16]; };

struct Value {
    union {
        uint64_t _i;
        double _d;
        void* _p;
    };
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
Value value_from_number(double d) { return { ._d = d }; /* TODO: check for nan and mask off! */}
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
Value value_from_function(FunctionType* func)    { return { nan_bits | type_bits_function | uint64_t(func) }; }
Value value_from_closure(ClosureType* closure)  { return { nan_bits | type_bits_closure | uint64_t(closure) }; }
Value value_from_boxed(BoxType* box)    { return { nan_bits | type_bits_boxed  | uint64_t(box) }; }
Value value_from_string(const StringType* str){ return { nan_bits | type_bits_string | uint64_t(str) }; }
Value value_from_object(ObjectType* obj){ return { nan_bits | type_bits_object | uint64_t(obj) }; }
Value value_from_array(ArrayType* arr)  { return { nan_bits | type_bits_array  | uint64_t(arr) }; }
Value value_from_vec2(vec2* v2)  { return { nan_bits | type_bits_vec2   | uint64_t(v2) }; }
Value value_from_vec3(vec3* v3)  { return { nan_bits | type_bits_vec3   | uint64_t(v3) }; }
Value value_from_vec4(vec4* v4)  { return { nan_bits | type_bits_vec4   | uint64_t(v4) }; }
Value value_from_mat4(mat4* m4)  { return { nan_bits | type_bits_mat4   | uint64_t(m4) }; }

// type checks
Type value_get_type(Value v)   { return isnan(v._d) ? (Type)(v._i & type_bits) : Type::Number; }
bool value_is_null(Value v)    { return v._i == UINT64_MAX; }
bool value_is_boolean(Value v) { return (v._i & type_bits) == type_bits_boolean; }
bool value_is_integer(Value v) { return (v._i & type_bits) == type_bits_integer; }
bool value_is_number(Value v)  { return !isnan(v._d); }
bool value_is_pointer(Value v) { return (v._i & type_bits) == type_bits_pointer; }
bool value_is_boxed(Value v)   { return isnan(v._d) && (v._i & type_bits) == type_bits_boxed;   }
bool value_is_function(Value v){ return (v._i & type_bits) == type_bits_function; }
bool value_is_closure(Value v) { return (v._i & type_bits) == type_bits_closure; }
bool value_is_string(Value v)  { return (v._i & type_bits) == type_bits_string;  }
bool value_is_object(Value v)  { return (v._i & type_bits) == type_bits_object;  }
bool value_is_array(Value v)   { return (v._i & type_bits) == type_bits_array;   }
bool value_is_vec2(Value v)    { return (v._i & type_bits) == type_bits_vec2; }
bool value_is_vec3(Value v)    { return (v._i & type_bits) == type_bits_vec3; }
bool value_is_vec4(Value v)    { return (v._i & type_bits) == type_bits_vec4; }
bool value_is_mat4(Value v)    { return (v._i & type_bits) == type_bits_mat4; }

StringType* value_to_string(Value v)        { return (StringType*)(v._i & pointer_bits); }
ArrayType* value_to_array(Value v)          { return (ArrayType*)(v._i & pointer_bits); }
ObjectType* value_to_object(Value v)        { return (ObjectType*)(v._i & pointer_bits); }
BoxType* value_to_box(Value v)              { return (BoxType*)(v._i & pointer_bits); }
FunctionType* value_to_function(Value v)    { return (FunctionType*)(v._i & pointer_bits); }
ClosureType* value_to_closure(Value v)      { return (ClosureType*)(v._i & pointer_bits); }
vec2* value_to_vec2(Value v)                { return (vec2*)(v._i & pointer_bits); }
vec3* value_to_vec3(Value v)                { return (vec3*)(v._i & pointer_bits); }
vec4* value_to_vec4(Value v)                { return (vec4*)(v._i & pointer_bits); }
mat4* value_to_mat4(Value v)                { return (mat4*)(v._i & pointer_bits); }
uint64_t value_to_null(Value v)             { return v._i; } // ???
bool value_to_boolean(Value v)              { return (bool)(v._i & boolean_bits); }
int32_t value_to_integer(Value v)           { return (int32_t)(v._i & integer_bits); }
double value_to_number(Value v)             { return v._d; } // ???
void* value_to_pointer(Value v)             { return (void*)(v._i & pointer_bits); }

ostream& operator<<(ostream& o, const mat4& v) { return o << "mat4 { ... }"; }
ostream& operator<<(ostream& o, const vec2& v) { return o << "vec2 { " << v.x << ", " << v.y << " }"; }
ostream& operator<<(ostream& o, const vec3& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << " }"; }
ostream& operator<<(ostream& o, const vec4& v) { return o << "vec2 { " << v.x << ", " << v.y << ", " << v.z << ", " << v.w << " }"; }

ostream& operator<<(ostream& o, const ArrayType& arr);
ostream& operator<<(ostream& o, const ObjectType& obj);
ostream& operator<<(ostream& o, const Value& v) {
    if (!isnan(v._d)) {
        return o << value_to_number(v);
    }
    switch (v._i & type_bits) {
        case (uint64_t)Type::Null:      return o << "null";
        case (uint64_t)Type::Boolean:   return o << (value_to_boolean(v) ? "true" : "false");
        case (uint64_t)Type::Integer:   return o << value_to_integer(v);
        case (uint64_t)Type::String:    return o << *value_to_string(v);
        case (uint64_t)Type::Pointer:   return o << value_to_pointer(v);
        case (uint64_t)Type::Object:    return o << "object {" << *value_to_object(v) << "}"; //*value_to_object(v);
        case (uint64_t)Type::Array:     return o << "array [" << *value_to_array(v) << "]"; //*value_to_array(v);
        case (uint64_t)Type::Function:  return o << "fn @ " << value_to_function(v)->code;
        case (uint64_t)Type::Mat4:      return o << *value_to_mat4(v);
        case (uint64_t)Type::Vec2:      return o << *value_to_vec2(v);
        case (uint64_t)Type::Vec3:      return o << *value_to_vec3(v);
        case (uint64_t)Type::Vec4:      return o << *value_to_vec4(v);
        case type_bits_boxed:           return o << *value_to_box(v);
        default:                        return o << "(unknown)" << hex << v._i << dec;
        // default: return o << value_to_number(v);
    }
}
ostream& operator<<(ostream& o, const ArrayType& arr) {
    if (arr.size()) {
        o << ' ' << arr[0];
        for (auto i = 1; i < arr.size(); i++) {
            o << ", " << arr[i];
        }
        o << ' ';
    }
    return o;
}
ostream& operator<<(ostream& o, const ObjectType& obj) {
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