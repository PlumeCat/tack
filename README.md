# TACK

## Building

```bash
mkdir build
cmake .. -G 'Unix Makefiles'
make
```

## TODO:

### Features
- [x] Arrays
- [x] Objects
- [ ] Closures
- [x] Const variables
- [ ] Calling C functions and vice versa
- [ ] Garbage collector
- [ ] Algebraics

### Other
Register based instead of stack

Performance improvement

Type-assuming versions of various instructions (eg ADD vs ADD_INT vs ADD_STR, ...)
    type deduction within a scope allows to emit different instructions
    this can be optionally expanded to full type system with proper enforcement