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
    // CFunction          = 0x00'0d'00'00'00'00'00'00
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

    /// @brief Underlying representation for strings
    struct StringType {
        std::string data;
        uint32_t refcount = 0;
        bool marker = false;
    };

    /// @brief Underlying representation for Arrays
    struct ArrayType {
        std::vector<TackValue> data;
        uint32_t refcount = 0;
        bool marker = false;
    };

    /// @brief Underlying representation for Objects
    struct ObjectType {
        KHash<std::string, TackValue> data;
        uint32_t refcount = 0;
        bool marker = false;
    };
    
    /// @brief Function signature for c functions which can be called by the VM through tack code
    using CFunctionType = TackValue(*)(class TackVM*, int, TackValue*);

    /// @brief Underlying representation for closures
    struct FunctionType {
        void* code_ptr; // pointer to CodeFragment, or pointer to CFunctionType
        bool is_cfunction = false;
        std::vector<TackValue> captures; // contains boxes
        uint32_t refcount = 0;
        bool marker = false;
    };



    /// @brief Return false if the value is 0.0, null or false, else true
    /// @details 0, null, false are falsy; everything else is truthy, **including empty string, empty object, null pointer, ...**
    /// @return 
    bool get_truthy();

    /// @brief Get a string representation for the value
    /// @details Not to be confused with `to_string()` !!
    /// @return 
    std::string get_string();

    // check type

    /// @brief Get the type of this value
    /// @return 
    inline TackType get_type() const                    { return std::isnan(_d) ? (TackType)(_i & type_bits) : TackType::Number; }

    /// @brief Check if this value is of type Null
    /// @return 
    inline bool is_null()      const                    { return _i == UINT64_MAX; }
    /// @brief Check if this value is of type Boolean
    /// @return 
    inline bool is_boolean()   const                    { return (_i & type_bits) == (uint64_t)TackType::Boolean; }
    /// @brief Check if this value is of type Number
    /// @return 
    inline bool is_number()    const                    { return !std::isnan(_d); }
    /// @brief Check if this value is of type Pointer
    /// @return 
    inline bool is_pointer()   const                    { return (_i & type_bits) == (uint64_t)TackType::Pointer; }
    /// @brief Check if this value is of type Function
    /// @return 
    inline bool is_function()  const                    { return (_i & type_bits) == (uint64_t)TackType::Function; }
    // /// @brief Check if this value is of type Cfunction
    // /// @return 
    // inline bool is_cfunction() const                    { return (_i & type_bits) == (uint64_t)TackType::CFunction; }
    /// @brief Check if this value is of type String
    /// @return 
    inline bool is_string()    const                    { return (_i & type_bits) == (uint64_t)TackType::String; }
    /// @brief Check if this value is of type Object
    /// @details TODO: returning true for numbers for some reason
    /// @return 
    inline bool is_object()    const                    { return (_i & type_bits) == (uint64_t)TackType::Object; }
    /// @brief Check if this value is of type Array
    /// @return 
    inline bool is_array()     const                    { return (_i & type_bits) == (uint64_t)TackType::Array; }


    /// @brief Get the underlying representation. UB if not string
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely segfault/exception) if the value is not of this type, so check first
    /// @return  
    inline StringType*      string()    const           { return (StringType*)(_i & pointer_bits); }
    /// @brief Get the underlying representation. UB if not array
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely segfault/exception) if the value is not of this type, so check first
    /// @return  
    inline ArrayType*       array()     const           { return (ArrayType*)(_i & pointer_bits); }
    /// @brief Get the underlying representation. UB if not object
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely segfault/exception) if the value is not of this type, so check first
    /// @return  
    inline ObjectType*      object()    const           { return (ObjectType*)(_i & pointer_bits); }
    /// @brief Get the underlying representation. UB if not function
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely segfault/exception) if the value is not of this type, so check first
    /// @return  
    inline FunctionType*    function()  const           { return (FunctionType*)(_i & pointer_bits); }
    
    /// @brief Get the underlying representation. UB if not cfunction
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely segfault/exception) if the value is not of this type, so check first
    /// @return  
    // inline CFunctionType    cfunction() const           { return (CFunctionType)(_i & pointer_bits); }
    
    /// @brief Get the underlying representation. UB if not boolean
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely nonsense return value) if the value is not of this type, so check first
    /// @return  
    inline bool             boolean()   const           { return (bool)(_i & 1u); }
    /// @brief Get the underlying representation. UB if not number
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely nonsense return value) if the value is not of this type, so check first
    /// @return  
    inline double           number()    const           { return _d; }
    /// @brief Get the underlying representation. UB if not pointer
    /// @details Convert to actual type.
    /// Undefined behaviour (most likely segfault/exception) if the value is not of this type, so check first
    /// Casting to underlying type might be possible, but not recommended.
    /// @return  
    inline void*            pointer()   const           { return (void*)(_i & pointer_bits); }

    // create value

    /// @brief Construct a new null value
    /// @return 
    static inline constexpr TackValue null()            { return { UINT64_MAX }; }
    /// @brief Construct a new boolean value
    /// @return 
    static inline constexpr TackValue false_()          { return { nan_bits | (uint64_t)TackType::Boolean }; }
    /// @brief Construct a new boolean value
    /// @return 
    static inline constexpr TackValue true_()           { return { nan_bits | (uint64_t)TackType::Boolean | 1 }; }
    /// @brief Construct a new number value from d
    /// @param d 
    /// @return 
    static inline TackValue number(double d)            { return { ._d = d }; /* TODO: check for nan and mask off? */ }
    /// @brief Construct a new boolean value from b
    /// @param b 
    /// @return 
    static inline TackValue boolean(bool b)             { return { nan_bits | (uint64_t)TackType::Boolean | (uint64_t)b }; }
    /// @brief Construct a new pointer value
    /// @details These pointers are opaque and are not used by Tack, but could be useful for passing values around the host program via Tack
    /// Tack does not manage/allocate/deallocate pointers with this value
    /// @param ptr 
    /// @return 
    static inline TackValue pointer(void* ptr)          { return { nan_bits | (uint64_t)TackType::Pointer | ((uint64_t)ptr & pointer_bits) }; }
    /// @brief Construct a new function value from the given underlying function. If a closure, captured values will be preserved.
    /// @param func 
    /// @return 
    static inline TackValue function(FunctionType* func){ return { nan_bits | (uint64_t)TackType::Function | uint64_t(func) }; }
    /// @brief Construct a new string value from the given underlying string
    /// @param str 
    /// @return
    static inline TackValue string(StringType* str)     { return { nan_bits | (uint64_t)TackType::String | uint64_t(str) }; }
    /// @brief Construct a new object value from the given underlying object
    /// @param obj 
    /// @return 
    static inline TackValue object(ObjectType* obj)     { return { nan_bits | (uint64_t)TackType::Object | uint64_t(obj) }; }
    /// @brief Construct a new array value from the given underlying array
    /// @param arr 
    /// @return 
    static inline TackValue array(ArrayType* arr)       { return { nan_bits | (uint64_t)TackType::Array | uint64_t(arr) }; }
    // /// @brief Construct a new cfunction value from the given function
    // /// @param cf 
    // /// @return 
    // static inline TackValue cfunction(CFunctionType cf) { return { nan_bits | (uint64_t)TackType::CFunction | uint64_t(cf) }; }

    /// @brief Test equality.
    /// @details Strings and numbers will be equivalent by value, everything else will be equivalent by identity
    /// @param r 
    /// @return 
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
    /// @brief Create a new TackVM. 
    /// @details Currently there is only one implementation - `Interpreter`, and the rest of the code is somewhat tied to this implementation. The TackVM interface is currently only used to hide the rest of the implementation details.
    /// You should deallocate the return value with `delete` once it is no longer needed, it can also be stored safely inside a `unique_ptr`
    /// @return Pointer to a new VM instance
    static TackVM* create();
    inline virtual ~TackVM() {}

    /// @brief Get the user pointer attached to this VM, or nullptr
    /// @return User pointer
    virtual void* get_user_pointer() const = 0;

    /// @brief Store a user-pointer inside the VM
    /// @details Can be used to store some program context and retrieve it from the VM from within a Cfunction. The VM will not modify this value
    /// @param ptr User pointer
    virtual void set_user_pointer(void* ptr) = 0;

    /// @brief Get the current state of the garbage collector
    /// @return 
    virtual TackGCState get_gc_state() const = 0;

    /// @brief Enable or disable the garbage collector
    /// @param state 
    virtual void set_gc_state(TackGCState state) = 0;

    // set a global variable

    /// @brief Set a global variable
    /// @details Roughly equivalent to `export let name = value` or `export const name = value`
    /// If a global variable by this name does not already exist, a new one will be defined.
    /// @param name Must be a valid identifier
    /// @param value 
    /// @param is_const If a new variable is created, whether to mark it const (so unable to be reassigned from tack code)
    virtual void set_global(const std::string& name, TackValue value, bool is_const = true) = 0;
    // set a global variable within a module namespace
    // this global can only be used by tack code if the module has been imported in the same tack scope or an enclosing scope

    /// @brief Set a global variable, within a module namespace
    /// @details Roughly equivalent to `export let name = value` or `export const name = value`
    /// If a global variable by this name does not already exist, a new one will be defined.
    /// If the module does not exist, it will be created. Avoid using module names that might conflict with tack source code filenames
    /// @param name Must be a valid identifier
    /// @param module_name Must be a valid identifier
    /// @param value 
    /// @param is_const If a new variable is created, whether to mark it const (so unable to be reassigned from tack code)
    virtual void set_global(const std::string& name, const std::string& module_name, TackValue value, bool is_const = true) = 0;

    // get global variables
    
    /// @brief Get a global variable
    /// @param name Name of the variable - must be a valid identifier
    /// @return Value of the global variable, or null if not found
    virtual TackValue get_global(const std::string& name) = 0;
    
    /// @brief Get a global variable
    /// @param name Name of the variable - must be a valid identifier
    /// @param module_name The module this variable is defined in
    /// @return Value of the global variable, or null if not found
    virtual TackValue get_global(const std::string& name, const std::string& module_name) = 0;
    
    /// @brief Get the type name associated with this type
    /// @param type Any type
    /// @return string-typed TackValue
    virtual TackValue get_type_name(TackType type) = 0;

    /// @brief Set up the standard library
    /// @details Once this has been called, any tack code in modules loaded by this VM can refer to any function from the standard library
    virtual void add_libs() = 0;

    /// @brief Add a module directory to be searched when an import statement is encountered
    /// @details If dir is empty, will add the current working directory
    /// @param dir
    virtual void add_module_dir(const std::string& dir = "") = 0;
    
    /// @brief Load and execute a Tack source file. 
    /// @param module_name Filename; should end in `.tack`
    virtual void load_module(const std::string& module_name) = 0;
    virtual TackValue call(TackValue fn, int nargs, TackValue* args) = 0;
    virtual void error(const std::string& msg) = 0;

    /// @brief Allocate a new object and return a pointer to it.
    /// @details The VM is responsible for deallocation
    virtual TackValue::ArrayType* alloc_array() = 0;

    /// @brief Allocate a new object and return a pointer to it.
    /// @details The VM is responsible for deallocation
    virtual TackValue::ObjectType* alloc_object() = 0;
    
    /// @brief Allocate a new string which will be garbage collected
    /// @details Use it for temp strings
    /// @param data Takes a copy of data
    /// @return 
    virtual TackValue::StringType* alloc_string(const std::string& data) = 0;

    /// @brief Allocate a new string which will live for as long as this VM instance. If this string has already been interned, the existing one will be returned.
    /// @details Use it for commonly-used strings like object keys, identifiers, string constants. Strongly recommended not to modify these once created.
    /// @param data Takes a copy of data
    /// @return 
    virtual TackValue::StringType* intern_string(const std::string& data) = 0;
};
