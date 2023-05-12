#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <exception>

#include "../src/khash2.h"

#define nan_bits        (0x7f'f0'00'00'00'00'00'00)
#define type_bits       (0x00'0f'00'00'00'00'00'00)
#define type_bits_boxed (0x00'0b'00'00'00'00'00'00)
#define pointer_bits    (0x00'00'ff'ff'ff'ff'ff'ff)
#define boolean_bits    (0x00'00'00'00'00'00'00'01)

enum class Type : uint64_t {
    Null               = 0x00'0f'00'00'00'00'00'00,
    Boolean            = 0x00'01'00'00'00'00'00'00,
    Number             = 0x00'0f'00'00'00'00'00'00,
    String             = 0x00'08'00'00'00'00'00'00,
    Pointer            = 0x00'00'00'00'00'00'00'00,
    Array              = 0x00'0a'00'00'00'00'00'00,
    Object             = 0x00'09'00'00'00'00'00'00,
    Function           = 0x00'0c'00'00'00'00'00'00,
    CFunction          = 0x00'0d'00'00'00'00'00'00,
};


enum class GCState : uint8_t {
    Disabled = 0,
    Enabled = 1,
};



struct Value {
    union {
        // TODO: undefined behaviour
        uint64_t _i;
        double _d;
        void* _p;
    };
    
    // Built-in types
    struct StringType {
        std::string data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    struct ArrayType {
        std::vector<Value> data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    struct ObjectType {
        KHash<std::string, Value> data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    struct FunctionType {
        struct CodeFragment* bytecode;
        std::vector<Value> captures; // contains boxes
        uint32_t refcount = 0;
        bool marker = false;
    };
    using CFunctionType = Value(*)(class TackVM*, int, Value*);

    // 0, null, false are falsy
    // everything else is truthy, including empty string, empty object, null pointer, etc
    bool get_truthy();
    // get the string representation (not to be confused with 'value_to_string' !)
    std::string get_string();
    
    inline Type get_type()                          { return std::isnan(_d) ? (Type)(_i & type_bits) : Type::Number; }
    inline bool is_null()                           { return _i == UINT64_MAX; }
    inline bool is_boolean()                        { return (_i & type_bits) == (uint64_t)Type::Boolean; }
    inline bool is_number()                         { return !std::isnan(_d); }
    inline bool is_pointer()                        { return (_i & type_bits) == (uint64_t)Type::Pointer; }
    inline bool is_function()                       { return (_i & type_bits) == (uint64_t)Type::Function; }
    inline bool is_cfunction()                      { return (_i & type_bits) == (uint64_t)Type::CFunction; }
    inline bool is_string()                         { return (_i & type_bits) == (uint64_t)Type::String; }
    inline bool is_object()                         { return (_i & type_bits) == (uint64_t)Type::Object; }
    inline bool is_array()                          { return (_i & type_bits) == (uint64_t)Type::Array; }

    #define check(ty) if (!is_##ty()) throw std::runtime_error("type error: expected " #ty);
    // convert to actual type
    inline StringType*      string()                { check(string);    return (StringType*)(_i & pointer_bits); }
    inline ArrayType*       array()                 { check(array);     return (ArrayType*)(_i & pointer_bits); }
    inline ObjectType*      object()                { check(object);    return (ObjectType*)(_i & pointer_bits); }
    inline FunctionType*    function()              { check(function);  return (FunctionType*)(_i & pointer_bits); }
    inline CFunctionType    cfunction()             { check(cfunction); return (CFunctionType)(_i & pointer_bits); }
    inline bool             boolean()               { check(boolean);   return (bool)(_i & boolean_bits); }
    inline double           number()                { check(number);    return _d; }
    inline void*            pointer()               { check(pointer);   return (void*)(_i & pointer_bits); }
    
    // create value
    static inline constexpr Value null()            { return { UINT64_MAX }; }
    static inline constexpr Value false_()          { return { nan_bits | (uint64_t)Type::Boolean }; }
    static inline constexpr Value true_()           { return { nan_bits | (uint64_t)Type::Boolean | 1 }; }
    static inline Value number(double d)            { return { ._d = d }; /* TODO: check for nan and mask off? */ }
    static inline Value boolean(bool b)             { return { nan_bits | (uint64_t)Type::Boolean | (uint64_t)b }; }
    static inline Value pointer(void* ptr)          { return { nan_bits | (uint64_t)Type::Pointer | ((uint64_t)ptr & pointer_bits) }; }
    static inline Value function(FunctionType* func){ return { nan_bits | (uint64_t)Type::Function | uint64_t(func) }; }
    static inline Value string(StringType* str)     { return { nan_bits | (uint64_t)Type::String | uint64_t(str) }; }
    static inline Value object(ObjectType* obj)     { return { nan_bits | (uint64_t)Type::Object | uint64_t(obj) }; }
    static inline Value array(ArrayType* arr)       { return { nan_bits | (uint64_t)Type::Array | uint64_t(arr) }; }
    static inline Value cfunction(CFunctionType cf) { return { nan_bits | (uint64_t)Type::CFunction | uint64_t(cf) }; }

    inline bool operator == (const Value& r) {
        if (std::isnan(_d) && std::isnan(r._d)) {
            return _i == r._i;
        }
        return _d == r._d;
    }
};


class TackVM {
public:
    static TackVM* create();
    inline virtual ~TackVM() {}    

    virtual void* get_user_pointer() const = 0;
    virtual void set_user_pointer(void* ptr) = 0;
    virtual GCState get_gc_state() const = 0;
    virtual void set_gc_state(GCState state) = 0;

    virtual void set_global(const std::string& name, Value value, bool is_const = true) = 0;
    virtual void set_global(const std::string& name, const std::string& module_name, Value value, bool is_const = true) = 0;
    virtual Value get_global(const std::string& name) = 0;
    virtual Value get_global(const std::string& name, const std::string& module_name) = 0;

    // set up the standard library
    void add_libs();
    
    // add a module directory to be searched when an import statement is encountered
    // if dir is empty, will add the current working directory
    virtual void add_module_dir(const std::string& dir = "") = 0;
    // load and execute a file
    virtual void load_module(const std::string& module_name) = 0;
    virtual Value call(Value fn, int nargs, Value* args) = 0;

    // Allocate a new array and return a pointer to it
    // Make sure to addref to prevent the array from being garbage collected
    virtual Value::ArrayType* alloc_array() = 0;

    // Allocate a new object and return pointer to it
    // Make sure to addref to stop the object being garbage collected
    virtual Value::ObjectType* alloc_object() = 0;
    
    // Allocate a new string which will be garbage collected
    // Use it for temp strings
    // Takes a copy of data
    virtual Value::StringType* alloc_string(const std::string& data) = 0;
    
    // Allocate a new string and intern it - the new string lifetime is tied to the interpreter lifetime
    // Takes a copy of data
    // Useful for commonly-used strings like object keys, identifiers, string constants
    virtual Value::StringType* intern_string(const std::string& data) = 0;
};
