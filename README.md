# TACK

## Example


For more code examples, see the `test` folder

## About

## Embedding

## Requirements

## Building

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

(ensure cmake is generating a Makefile with `cmake .. -G 'Unix Makefiles')

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
- [ ] refactor `Value`
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
