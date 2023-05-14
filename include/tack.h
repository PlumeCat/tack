#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "../src/khash2.h"

#define nan_bits        (0x7f'f0'00'00'00'00'00'00)
#define type_bits       (0x00'0f'00'00'00'00'00'00)
#define pointer_bits    (0x00'00'ff'ff'ff'ff'ff'ff)

enum class TackType : uint64_t {
    Null               = 0x00'0f'00'00'00'00'00'00,
    Boolean            = 0x00'01'00'00'00'00'00'00,
    Number             = 0x00'03'00'00'00'00'00'00,
    String             = 0x00'08'00'00'00'00'00'00,
    Pointer            = 0x00'00'00'00'00'00'00'00,
    Array              = 0x00'0a'00'00'00'00'00'00,
    Object             = 0x00'09'00'00'00'00'00'00,
    Function           = 0x00'0c'00'00'00'00'00'00,
    CFunction          = 0x00'0d'00'00'00'00'00'00
};

enum class TackGCState : uint8_t {
    Disabled = 0,
    Enabled = 1,
};

struct TackValue {
    union {
        // TODO: undefined behaviour
        uint64_t _i;
        double   _d;
        void*    _p;
    };

    // Built-in types
    struct StringType {
        std::string data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    struct ArrayType {
        std::vector<TackValue> data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    struct ObjectType {
        KHash<std::string, TackValue> data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    struct FunctionType {
        struct CodeFragment* bytecode;
        std::vector<TackValue> captures; // contains boxes
        uint32_t refcount = 0;
        bool marker = false;
    };
    using CFunctionType = TackValue(*)(class TackVM*, int, TackValue*);

    // 0, null, false are falsy
    // everything else is truthy, including empty string, empty object, null pointer, etc
    bool get_truthy();
    // get the string representation (not to be confused with 'to_string' !)
    std::string get_string();

    // check type
    inline TackType get_type() const                    { return std::isnan(_d) ? (TackType)(_i & type_bits) : TackType::Number; }
    inline bool is_null()      const                    { return _i == UINT64_MAX; }
    inline bool is_boolean()   const                    { return (_i & type_bits) == (uint64_t)TackType::Boolean; }
    inline bool is_number()    const                    { return !std::isnan(_d); }
    inline bool is_pointer()   const                    { return (_i & type_bits) == (uint64_t)TackType::Pointer; }
    inline bool is_function()  const                    { return (_i & type_bits) == (uint64_t)TackType::Function; }
    inline bool is_cfunction() const                    { return (_i & type_bits) == (uint64_t)TackType::CFunction; }
    inline bool is_string()    const                    { return (_i & type_bits) == (uint64_t)TackType::String; }
    inline bool is_object()    const                    { return (_i & type_bits) == (uint64_t)TackType::Object; }
    inline bool is_array()     const                    { return (_i & type_bits) == (uint64_t)TackType::Array; }

    // convert to actual type. result will be undefined (most likely crash) if the value's type does not match, so check first
    inline StringType*      string()    const           { return (StringType*)(_i & pointer_bits); }
    inline ArrayType*       array()     const           { return (ArrayType*)(_i & pointer_bits); }
    inline ObjectType*      object()    const           { return (ObjectType*)(_i & pointer_bits); }
    inline FunctionType*    function()  const           { return (FunctionType*)(_i & pointer_bits); }
    inline CFunctionType    cfunction() const           { return (CFunctionType)(_i & pointer_bits); }
    inline bool             boolean()   const           { return (bool)(_i & 1u); }
    inline double           number()    const           { return _d; }
    inline void*            pointer()   const           { return (void*)(_i & pointer_bits); }

    // create value
    static inline constexpr TackValue null()            { return { UINT64_MAX }; }
    static inline constexpr TackValue false_()          { return { nan_bits | (uint64_t)TackType::Boolean }; }
    static inline constexpr TackValue true_()           { return { nan_bits | (uint64_t)TackType::Boolean | 1 }; }
    static inline TackValue number(double d)            { return { ._d = d }; /* TODO: check for nan and mask off? */ }
    static inline TackValue boolean(bool b)             { return { nan_bits | (uint64_t)TackType::Boolean | (uint64_t)b }; }
    static inline TackValue pointer(void* ptr)          { return { nan_bits | (uint64_t)TackType::Pointer | ((uint64_t)ptr & pointer_bits) }; }
    static inline TackValue function(FunctionType* func){ return { nan_bits | (uint64_t)TackType::Function | uint64_t(func) }; }
    static inline TackValue string(StringType* str)     { return { nan_bits | (uint64_t)TackType::String | uint64_t(str) }; }
    static inline TackValue object(ObjectType* obj)     { return { nan_bits | (uint64_t)TackType::Object | uint64_t(obj) }; }
    static inline TackValue array(ArrayType* arr)       { return { nan_bits | (uint64_t)TackType::Array | uint64_t(arr) }; }
    static inline TackValue cfunction(CFunctionType cf) { return { nan_bits | (uint64_t)TackType::CFunction | uint64_t(cf) }; }

    inline bool operator == (const TackValue& r) const {
        if (std::isnan(_d) && std::isnan(r._d)) {
            if (is_string() && r.is_string()) {
                return string()->data == r.string()->data;
            }
            return _i == r._i;
        }
        return _d == r._d;
    }
};

class TackVM {
public:
    static TackVM* create();
    inline virtual ~TackVM() {}

    // get user pointer - useful to store some context and retrieve it inside a Cfunction
    virtual void* get_user_pointer() const = 0;
    // set user pointer - useful to store some context and retrieve it inside a Cfunction
    virtual void set_user_pointer(void* ptr) = 0;

    // get the current state of the GC
    virtual TackGCState get_gc_state() const = 0;

    // set the current state of the GC
    virtual void set_gc_state(TackGCState state) = 0;

    // set a global variable
    virtual void set_global(const std::string& name, TackValue value, bool is_const = true) = 0;
    // set a global variable within a module namespace
    // this global can only be used by tack if the module has been imported in the same tack scope
    virtual void set_global(const std::string& name, const std::string& module_name, TackValue value, bool is_const = true) = 0;

    // get global variables
    virtual TackValue get_global(const std::string& name) = 0;
    virtual TackValue get_global(const std::string& name, const std::string& module_name) = 0;
    virtual TackValue get_type_name(TackType type) = 0;

    // set up the standard library
    virtual void add_libs() = 0;

    // add a module directory to be searched when an import statement is encountered
    // if dir is empty, will add the current working directory
    virtual void add_module_dir(const std::string& dir = "") = 0;
    // load and execute a file
    virtual void load_module(const std::string& module_name) = 0;
    virtual TackValue call(TackValue fn, int nargs, TackValue* args) = 0;
    virtual void error(const std::string& msg) = 0;

    // Allocate a new array and return a pointer to it
    // Make sure to addref to prevent the array from being garbage collected
    virtual TackValue::ArrayType* alloc_array() = 0;

    // Allocate a new object and return pointer to it
    // Make sure to addref to stop the object being garbage collected
    virtual TackValue::ObjectType* alloc_object() = 0;

    // Allocate a new string which will be garbage collected
    // Use it for temp strings
    // Takes a copy of data
    virtual TackValue::StringType* alloc_string(const std::string& data) = 0;

    // Allocate a new string and intern it - the new string lifetime is tied to the interpreter lifetime
    // Takes a copy of data
    // Useful for commonly-used strings like object keys, identifiers, string constants
    virtual TackValue::StringType* intern_string(const std::string& data) = 0;
};
