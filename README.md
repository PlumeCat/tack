# TACK

## Building

```bash
mkdir build
cmake .. -G 'Unix Makefiles'
make
```


## TODO:

### Features
Objects
Closures
C functions
Algebraics
Garbage collector

### Other
Try the lua sliding register window approach, maybe only 16 registers - fit into 4 bits

Type-assuming versions of various instructions (eg ADD vs ADD_INT vs ADD_STR, ...)
    type deduction within a scope allows to emit different instructions
    will need the compiler to do stack accumulator again
    this can be optionally expanded to full type system with proper enforcement