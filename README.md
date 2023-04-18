# TACK

## Building

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

(ensure cmake is generating a Makefile with `cmake .. -G 'Unix Makefiles')


## TODO

### Features
- [x] variadic print
- [x] a more usable looping construct
- [ ] break and continue? labelled break?
- [x] arrays
- [x] objects
- [x] comments
- [x] closures
- [x] consts
- [x] calling C functions from tack
- [ ] comments
- [ ] boolean literals and operations (do as number?)
- [ ] string operations
- [x] calling tack functions from C
- [x] garbage collector. must be allowed to retain pointers in C world
- [ ] algebraics
- [ ] some kind of error handling (note: lua has pcall(func) not true exceptions)
- [x] initialization for array and object literals
- [ ] proper module system
- [ ] python style slice syntax for arrays
- [x] allow more than 256 constants
- [ ] language standard library, doesn't have to be massive
- [ ] write a small game / opengl binding to test

### Undecided
- [ ] comparisons; '==' vs python style 'is'
- [ ] type coercions
- [ ] separate type for integers worth it? complicates number handling and can only do 32 or 48 bit
- [ ] deleting keys from objects (currently impossible have to generate a new object)


### improvement & optimization
- [ ] growable stack / configurable stack size
- [ ] automatically detect infinite loops / infinite recursion
- [ ] tail calls
- [ ] for loops over functions (null to terminate)
- [ ] compile time boxing and move elission
- [x] ensure const is respected for captures
- [ ] deduplicate consts
	- if possible, dedupe across fragments
- [x] specialize global lookups, treat it like LOAD_CONST (will want more than 256 global lookups)
- [ ] review notes and apply optimizations
- [ ] possibly intern strings - definitely intern/prehash identifiers
- [ ] replace CONDJUMP with something like CONDSKIP (see notes)
- [ ] type specialized instructions
- [ ] poor man's JIT: loop over instructions and emit machine code corresponding to a direct function call per opcode
	- believe this is called a templating JIT, performance might be OK
	- for extra points, write/disassembly the ASM for each opcode and emit it inline, elide the calls



### developer qol
- [ ] compile as static lib for embeddability
- [ ] a nice REPL
- [ ] debugging facilities. maybe even a full debugger. interop with vscode
- [ ] language server