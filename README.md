# TACK

## Building

```bash
mkdir build
cmake .. -G 'Unix Makefiles'
make
```

## TODO:

- [ ] Chunk based compile functions as they are encountered instead of pushing to queue
	This will simplify lookups massively as scopes can go back to being a stack
	instead of tracking all scopes and keeping a tree

	Also simplifies closures


### Features
- [ ] Proper for loops
- [x] Arrays
- [x] Objects
- [x] Comments
- [ ] Closures
- [x] Const variables
- [ ] Calling C functions and vice versa
- [ ] Garbage collector
- [ ] Algebraics

### Other
- [ ] review notes and apply optimizations
- [ ] poor man's JIT: loop over instructions and emit machine code corresponding to a direct function call per opcode
- [ ] deduplicate consts
- [ ] possibly intern strings - definitely intern identifiers
- [ ] allow more than 256 constants (!)
- [ ] initialization for array and object literals
- [ ] Compile as library
- [ ] QOL improvements
- [ ] Language standard library