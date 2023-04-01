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

### Other
- [ ] mark boxed registers and elide the box check with every register access. should allow to remove the box type
- [ ] initialization for array and object literals
- [ ] review notes and apply optimizations
- [ ] ensure const is respected for captures
- [ ] replace CONDJUMP with something like CONDSKIP (see notes)
- [ ] deduplicate consts
	- if possible, dedupe across fragments
- [ ] possibly intern strings - definitely intern identifiers
- [x] allow more than 256 constants
- [ ] compile as static lib for embeddability
- [ ] a nice REPL
- [ ] language standard library, doesn't have to be massive
- [ ] python style slice syntax
- [ ] write a small game to test
- [ ] poor man's JIT: loop over instructions and emit machine code corresponding to a direct function call per opcode
	- believe this is called a templating JIT, performance might be OK
	- for extra points, write/disassembly the ASM for each opcode and emit it inline, elide the calls
- [ ] need to think about comparisons; '==' vs python style 'is'
- [ ] need to think about coercions
- [ ] need to think about are 32 bit signed integers worth it?
- [ ] type specialized instructions
- [ ] debugging facilities. maybe even a full debugger. interop with vscode
- [ ] language server