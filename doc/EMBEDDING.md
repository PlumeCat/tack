## Embedding

##### You must use C++20 (or greater)

Tack can easily be embedded into an existing C++ program, either by linking the static library, or by adding the `src/` and `include/` folders to the project.

In either case, the steps involved are:

- include the header `include/tack.h`: this header contains the entire public interface for Tack.
- create an instance of `TackVM`; this is currently an abstract interface implemented by one interpreter class, so use `TackVM::create()`
- (optional) Add the standard library, and bind any values from the host program
- use `TackVM::load_module()` to execute tack source code from a file.
- after the vm has finished executing the module, it can be expected that any transient values have been deallocated; the host program may continue interacting with retained values.

## Example

Minimal C++ example that starts a VM, binds the standard library, executes a single source file, and exits.

```c++
#include <tack.h>

int main(int argc, char* argv[]) {
    auto vm = TackVM::create();
    vm->add_libs(); // make the standard library accessible to tack code
    vm->add_module_directory(); // adds the cwd as a module search directory
    vm->load_module("source.tack"); // load and execute "source.tack"
    delete vm;
    return 0;
}
```

Further examples on this page can be considered to inherit from this example.

---

## Passing values to/from Tack and the host program

Whilst the ability to execute single files is not without its uses, they pale in comparison with the ability to pass arbitrary values to and from a host program.
There are several ways the host program can pass values into tack code, or read values back:

- global variables: see `TackVM::get_global()`
- object and array values: see `Iterating objects and arrays from C++`
- cfunction arguments and return values: see `Calling Tack functions from C++ code`
- tack function arguments and return values: see `Calling C++ functions from Tack code`
- allocating new values from the host: this applies to strings, arrays, and objects.
see `TackVM::alloc_string()`, `TackVM::alloc_object()`, `TackVM::alloc_array()`

#### Note: Pointers

The pointer type is similar to Lua's lightuserdata; the host program can pass it into the Tack VM for storage from whence it may be passed around Tack code arbitrarily, and passed back to the host program elsewhere. There is little use for this at present, pending the implementation of vtables.

---

## Calling Tack functions from C++ code

A tack function is a `TackValue` like any other value, so the host program just has to pass a function-typed `TackValue` into `TackVM::call()` along with the arguments. The return value from the function is returned as a `TackValue` from `TackVM::call()` to the host program.

```rust
" tack code "
export fn say_hello(name, x) {
    print("hello", name)
    return x + 2
}
```
```c++
TackValue args[2] = {
    TackValue::alloc_string(vm->alloc_string("friend")),
    TackValue::number(8999),
};
auto value = vm->get_global("say_hello");
vm->call(value, 2, args); // TackValue::number(9001)
```

---
## Calling C++ functions from tack code

Tack can call C++ functions with the `TackValue::CFunctionType` signature:

`TackValue (*)(TackVM* vm, int nargs, TackValue* args)`

 - `vm`: The TackVM that initiated the call
 - `nargs`: The number of arguments passed into the function
 - `args`: The arguments.

 Note that the `args` parameter may be pointing directly into the Tack stack; it is the host program's responsibility not to write past the end of `args[nargs-1]`, doing so can be considered undefined behaviour (but will most likely cause an error and/or crash in the Tack program).

 Writing to the `args` array currently has no effect as arguments are re-pushed to the stack **but should be avoided in general**, in future versions it may be impossible/cause errors

 From tack, a C function is represented as a first-class value, so it can be passed around, copied, etc freely. They are called from Tack in the same manner as normal Tack functions

```c++
// C++
TackValue my_tack_func(TackVM* vm, int nargs, TackValue* args) {
    std::cout << "Hello Tack from C++!" << std::endl;
    std::cout << "Received: " << nargs << "arguments" << std::endl;
    for (auto i = 0; i < nargs; i++) {
        std::cout << "Argument: " << args[i].get_string() << std::endl;
    }
}
vm->set_global("my_func", TackValue::cfunction(my_tack_func));
```
```rust
" tack code "
my_func(1, 2, "hello")
```

---

## Retaining Tack data in C++ code

A key difference between Tack and  Lua is the ability to "retain" direct pointers to Tack data in the host program. The idea behind this is to avoid the bottleneck caused Lua-style stack based interop.

Usually if a function, object or other has gone out of scope in the Tack code and is no longer reachable through any live objects, it would be expected that the garbage collector (GC) will deallocate it at some point. However, a reference counting mechanism is provided _in addition to_ the usual mark-and-sweep GC; the host can increment a value's refcount to indicate that an object is not to be deallocated by the Tack GC. In addition, values reachable through the refcounted object will be kept alive as well, even if they are not refcounted themselves.

Values of the following types can be retained by C++ code: `object`, `array`, `string`, `function`. To retain a value, unpack it to the corresponding underlying type (one of `TackValue::*Type`) and set the refcount to a non-zero value. The host program is free to set the refcount value to anything, the term "refcount" is merely a suggestion as to the intended usage! Tack will simply observe for the refcount being set back to 0, at which point the value is eligible for garbage collection.

#### Example

```c++
std::vector<TackValue::FunctionType*> callbacks;

TackValue set_callback(TackVM* vm, int nargs, TackValue* args) {
    if (args[0].is_function()) {
        auto func = args[0].function();
        func->refcount++;
        callbacks.emplace_back(func);
    } else {
        vm->error("set_callback(): expected a function");
    }
}

TackValue trigger_callbacks(TackVM* vm, int nargs, TackValue* args) {
    for (auto c: callbacks) {
        vm->call(c, 0, nullptr);
    }
}


TackValue cleanup_callbacks(TackVM* vm, int nargs, TackValue* args) {
    for (auto c: callbacks) {
        c->refcount = 0; // important - set the ref count to 0 when the function is no longer needed!
    }
    callbacks.clear();
}

int main() {
    ...
    vm->set_global("set_callback", TackValue::cfunction(set_callback), true);
    vm->set_global("trigger_callbacks", TackValue::cfunction(trigger_callbacks), true);
    vm->set_global("cleanup_callbacks", TackValue::cfunction(cleanup_callbacks), true);
    vm->load_module("source.tack");
    vm->load_module("source2.tack");
    ...
}
```

```rust
" source.tack "
set_callback(fn() { print("callback 1") })
set_callback(fn() { print("callback 2") })
let temp_obj = { x = 42 }
set_callback(fn() { print("callback 3", temp_obj.x) })

```
```rust
" source2.tack "
trigger_callbacks()
"output, assuming obj was not modified:
    callback 1
    callback 2
    callback 3 42
"

cleanup_callbacks()
trigger_callbacks() " no output "
```

In this example, the tack functions passed into the host program are kept alive by it; as temporary values they would be inaccessible past the end of the `source.tack` module and so would be deallocated if not for the host program incrementing the refcount.

In particular, `temp_obj` is also out of scope at the end of `source.tack`. As it is captured by 
callback 3, and callback 3 is kept alive by the host program, `temp_obj` will only be deallocated after callback 3 refcount goes back to 0 in `cleanup_callbacks()`

---

## Iterating objects and arrays from C++

```C++
// "value" is a TackValue

// Iterating an array
if (value.is_array()) {
    auto* arr = value.array();

    // with index
    for (auto i = 0u; i < arr->data.size(); i++) {
        auto val = arr->data[i];
        std::cout << "Array element: " << i << " - " << val.get_string() << std::endl;
    }
    
    // without index
    for (auto val: arr->data) {
        std::cout << "Array element: " << val.get_string() << std::endl;
    }
}

// Iterating an object
if (value.is_object()) {
    auto* obj = value.object();

    // Iterate over the underlying KHash
    for (auto i = obj->data.begin(); i != obj->data.end(); i = obj->data.next(i)) {
        auto& key = obj->key_at(i); // key is std::string&
        auto val = obj->value_at(i);// val is another TackValue

        std::cout << "Object: key: " << key << " - value: " << val.get_string() << std::endl;
    }
}
```

Be careful when recursively iterating objects/arrays as there could be cycles. An object value/array element may even reference the object or array itself.

---

## Notes

- Non-capturing C++ lambdas can be passed into Tack if they have the right signature: this can make binding a little less arduous

    ```c++
    vm->set_global("lambda", [](TackVM* vm, int, TackValue*) {
        std::cout << "Hello Tack from lambda!" << std::endl;
    }, true);
    ```

- In the callback example, it would also be valid to keep a list of TackValue instead of TackValue::FunctionType, however with this approach the type check and the unpacking of the value into a `FunctionType` only needs to be done once.

- The underlying data for objects is provided by `KHash`, a C++ port of the excellent khash library - see `src/khash2.h` [sorry, it's undocumented for the time being]. The data can be found under `TackValue::ObjectType::data`. The host program can modify the values here, even add/remove keys.

- The underlying storage for arrays is provided by `std::vector` and can be found under `TackValue::ArrayType::data`. The host program can modify this data inplace*

- The underlying storage for strings is provided by `std::string`, and can be found under `TackValue::StringType::data`. The host program can modify this data inplace*

- The captured values for a closure can be found under `TackValue::FunctionType::captures`; however the values here are indirect; the lower 48 bits is a pointer to a special "hidden" type Box:

    ```C++
    struct BoxType {
        TackValue value;
        bool marker = false;
    };
    ```

    with the `value` field containing the actual value.

    The host program could modify this value*, but it is not immediately apparent why this would be desirable. 


\* Modifying this data is not recommended, but if needs must, it should be done in mutual exclusion from the owning TackVM, ie don't do it on another thread while the TackVM is running.