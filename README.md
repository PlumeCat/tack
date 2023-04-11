# TACK

## Building

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

(ensure cmake is generating a Makefile with `cmake .. -G 'Unix Makefiles')

### Features
- [ ] variadic print
- [ ] a more usable looping construct
- [ ] break and continue? labelled break?
- [x] arrays
- [x] objects
- [x] comments
- [x] closures
- [x] consts
- [ ] calling C functions and vice versa
- [ ] garbage collector. must be allowed to retain pointers in C world
- [ ] algebraics
- [ ] provision for error handling (note: lua has pcall(func) not true exceptions)
- [x] initialization for array and object literals
- [ ] module system or `export` or `dofile()` or otherwise 
	- different files should be able to access each other's globals

### Other
- [ ] replace CONDJUMP with something like CONDSKIP (see notes)
- [ ] review notes and apply optimizations
- [ ] ensure const is respected for captures
- [ ] deduplicate consts
	- if possible, dedupe across fragments
- [ ] possibly intern strings - definitely intern/prehash identifiers
- [x] allow more than 256 constants
- [ ] specialize global lookups, treat it like LOAD_CONST (will want more than 256 global lookups)
- [ ] compile as static lib for embeddability
- [ ] a nice REPL
- [ ] language standard library, doesn't have to be massive
- [ ] python style slice syntax for arrays
- [ ] write a small game / opengl binding to test
- [ ] poor man's JIT: loop over instructions and emit machine code corresponding to a direct function call per opcode
	- believe this is called a templating JIT, performance might be OK
	- for extra points, write/disassembly the ASM for each opcode and emit it inline, elide the calls
- [ ] need to think about comparisons; '==' vs python style 'is'
- [ ] need to think about coercions
- [ ] need to think about are 32 bit signed integers worth it?
- [ ] type specialized instructions
- [ ] debugging facilities. maybe even a full debugger. interop with vscode
- [ ] language server