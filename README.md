# Tack

A fast lightweight scripting language for games and embedding

```rust
fn quicksort(arr) {
    const n = #arr
    if n <= 1 {
        return arr
    }

    " find the midpoint "
    let l = min(arr)
    let r = max(arr)
    if r == l {
        return arr
    }
    const mid = (l + r) / 2
    
    " split array into upper and lower "
    const upper = []
    const lower = []
    for i in 0, n {
        const e = arr[i]
        if e < mid {
            lower << e
        } else {
            upper << e
        }
    }

    " recursively sort the upper and lower sub-arrays and join the result"
    return quicksort(lower) + quicksort(upper)
}

let A = []
for i in 1, 1000000 {
    A << random()
}
let B = quicksort(A)
```

For a detailed look at the language itself, see [Introduction to Tack](doc/INTRODUCTION.md)

Tack is semantically a mixture of Lua, Javascript and Python, with some Rust influence on the syntax. It aims for performance equal to or better than the stock Lua interpreter, while offering alternative syntax, adjusted semantics, and much more flexibility for integrating into host C++ programs.

Key features:
- Dynamic typing
- Garbage collection
- Lexically scoped closures
- "Register"-based bytecode
- Hybrid stackless interpreter

To run a script, build the `tack` executable, then:

```bash
tack my-script.tack
```

## Requirements

- cmake
- doxygen (optional)

## Building

```bash
# After cloning the repo into 'tack/'
cd tack
mkdir build
cd build
cmake ..
make # Generates the `tack` executable in the `build/` folder
```
To force CMake to generate a Makefile: `cmake .. -G 'Unix Makefiles` . However, the provided CMakeLists should also be usable in Visual Studio via the "Open Folder" option

For detailed language documentation see the `doc/` folder

Generate documentation (recommended) for the public C++ interface by running `doxygen` in the root. Documentation is then found in `doc/html/index.html`



---
### Motivations

A sole developer's learning project! With the ambition to improve and be more widely used.
This project is relatively young, and should be considered "v0.1" alpha-level/early beta-level software.
The compiler and parser in particular may have bugs or subtly incorrect behaviour, and are in need of more battle-testing before this can be recommended for use in serious production contexts.

Why not use Python?
- performance
- better variable scoping; `let` keyword required for new variables.

Why not use Lua?
- inefficiency due to stack based binding
- some operations such as array length are unexpectedly O(N)
- some operations like array length are unintuitively defined
- inconsistency of invoking a "method" from a metatable**: `foo:bar()` vs `foo.bar()`
    - tack does not have a metatable/prototype equivalent yet. Basic objects can be implemented via closures, the upcoming "vtables" feature will canonicalize this and be more efficient.
- syntax, although this is personal preference more than anything.

Why not use LuaJIT?
- ...You probably should use LuaJIT.

Why not use NodeJS?
- https://www.destroyallsoftware.com/talks/wat

**Why not use Tack?**

- Immature
- No proper comments - loose string literals are a surprisingly adequate substitute though
- Very limited error handling
- Limited support, no literature
- No unicode yet
- No coroutines/parallelism
- Limited ergonomicity of the standard library, and some batteries not included. (No file/socket IO yet)
- Syntax and standard library are unfixed, possibly self-inconsistent or ill-defined, and likely to change. See [Syntax](doc/SYNTAX.md)
    
_Despite this, it is already possible to build small games and useful scripts with tack in its current state!_

---

### Upcoming features

- [x] variadic print
- [x] a more usable looping construct
- [ ] break and continue? labelled break?
- [ ] comments
- [x] closures
- [x] consts
- [x] calling C/C++ functions from tack
- [x] calling tack functions from C/C++
- [x] boolean literals and operations
- [x] string operations (+, #, ...)
- [ ] documentation for embedding
- [x] garbage collector
- [ ] improve error handling and stack traces
- [x] initialization for array and object literals
- [x] basic module system
- [ ] python style slice syntax for arrays
- [ ] python style for comprehensions
- [x] allow more than 256 constants
- [x] write a small game / opengl binding to test
- [ ] floor division
- [x] refactor `Value`
- [ ] `self` or `this` keyword in object literals?
- [ ] double check semantics around comparisons; '==' vs 'is'
- [ ] "vtable"-alike implementation to allow method calls
- [ ] refactor standard library with vtables
- [ ] implement CObject with vtables
- [ ] reimplement all non trivial types with CObject (?)
- [ ] more standard library (io, datastructures, ...)
- [ ] growable stack
- [ ] tail call optimization
- [ ] for loops over functions (null to terminate)
- [ ] compile time boxing
- [ ] type deduction in AST for optimizations
- [ ] lifetime/escape analysis
- [ ] improve GC
- [ ] JIT
