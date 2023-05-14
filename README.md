# TACK

A small scripting language for games and lightweight embedding

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

" test "
let A = []
for i in 1, 1000000 {
    A << random()
}
let B = quicksort(A)
```

Tack is semantically a mixture of Javascript, Lua and Python, with some Rust influence on the syntax. It aims to perform similarly to Lua without

Features: dynamic typing, closures, fast interpreter, efficient binding

To run a script:

```bash
tack my-script.tack
```

Why tack and not python

- faster than python, probably
- let keyword for variables, and better scopes
- lightweight

Why tack and not lua
- binding pain with the stack; keep a pointer to arguments, keep a pointer to values/data
- 1-based indexing
- inconsistency with metatable** method invocation: `foo:bar()` vs `foo.bar()`
- pascal syntax
- node wins for performance

Why tack and not node
- https://www.destroyallsoftware.com/talks/wat


## Building

Tested with: MSVC 2022, clang 14, g++ 11

```bash
# After cloning the repo into 'tack/'
cd tack
mkdir build
cd build
cmake ..
make # Generates the `tack` executable in the `build/` folder
```
To force CMake to generate a Makefile: `cmake .. -G 'Unix Makefiles` . However, the provided CMakeLists should also be usable in Visual Studio via the "Open Folder" feature

For more detailed language documentation see the `doc/` folder

Generate documentation (recommended) for the public C++ interface by running `doxygen` in the root. Documentation is then found in `doc/html/index.html`


## Todo List
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
- [ ] templating JIT
