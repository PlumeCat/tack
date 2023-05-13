## Embedding

Tack can easily be embedded into an existing C++ program, either by linking the static library, or by adding the `src/` and `include/` folders to the project.

In either case, the only header that needs to be included is `include/tack.h`

The program must then create an instance of the `TackVM` class, interaction with the class is 

## Example

Minimal C++ example that starts a VM, binds the standard library, executes a single source file, and exits.

```c++
#include <tack.h>

int main(int argc, char* argv[]) {
    auto vm = TackVM();
    vm->add_libs(); // make the standard library accessible to tack code
    vm->add_module_directory(); // adds the cwd as a module search directory
    vm->load_module("source.tack"); // load and execute "source.tack"
    delete vm;
    return 0;
}
```

Further examples in this document can be considered to inherit from this example.

---

## Calling global Tack functions from C++ code

To call a tack function, it must be first be reachable via `get_global` (marked with the `export` keyword), reached through another host-persistent tack object, or passed into a host function.

Example:

```rust
" source.tack "
let a = [
    fn() { print("tack 1") },
    fn() { print("tack 2") }
]
handle_func_array()
```

---
## Calling C++ functions from tack code

Tack can call C++ functions with the `TackValue::CFunctionType` signature:

`TackValue (*)(TackVM* vm, int nargs, TackValue* args)`

 - `vm`: The TackVM that initiated the call
 - `nargs`: The number of arguments passed into the function
 - `args`: The arguments.

 The args parameter may point directly into the Tack stack; it is the program's responsibility not to write past the end of `args[nargs-1]`, doing so can be considered undefined behaviour (but will most likely cause an exception in the Tack program shortly afterward).

There are several ways to make a CFunction accessible to the tack source code. The easiest is to bind as a global variable (or const)

```c++

TackValue my_tack_func(TackVM* vm, int nargs, TackValue* args) {
    std::cout << "Hello Tack from C++!" << std::endl;
}

...
vm->set_global("my_func", TackValue::cfunction(my_tack_func)
```

---

## Retaining Tack objects in C++ code

A key difference between Tack and  Lua is the ability to keep direct pointers to Tack values from the host program. The idea behind this is to avoid the bottleneck caused Lua-style stack based interop.

Values can either be looked up by the host program with `get_global`, passed into the host program via function calls, returned from tack functions, extracted from tack objects/arrays.

Usually if a function, object or other has gone out of scope in the Tack code and is no longer reachable through any live objects, it would be expected that the garbage collector will deallocate it at some point. However, a reference counting mechanism is provided _in addition to_ the usual mark-and-sweep garbage collector; the host can use the reference count to indicate that an object is not to be deallocated by the Tack VM. In addition, values reachable through the refcounted object will be kept alive as well, even if they are not refcounted themselves, so that the refcounted object can use them as intended.

In this example, the tack functions passed into the host program are kept alive by it.

### Example

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
    ...
}
```

```rust
" source.tack "
set_callback(fn() { print("callback 1") })
set_callback(fn() { print("callback 2") })
let obj = { x = 42 }
set_callback(fn() { print("callback 3", obj.x) })

trigger_callbacks()
"output, assuming obj was not modified:
    callback 1
    callback 2
    callback 3 42
"

cleanup_callbacks()
trigger_callbacks() " no output "
```

In particular, the `obj` value is also kept alive by the capturing callback, long after the tack code module has finished executing.

---
## Notes

- Non-capturing C++ lambdas can be passed into Tack if they have the right signature: this can make binding a little less arduous

    ```c++
    vm->set_global("lambda", [](TackVM* vm, int, TackValue*) {
        std::cout << "Hello Tack from lambda!" << std::endl;
    }, true);
    ```

- In the callback example, it would also be valid to keep a list of TackValue instead of TackValue::FunctionType, however with this approach the type check and the unpacking of the value into the function pointer only needs to be done once.