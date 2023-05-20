# Introduction to Tack

## Values and types

Values must be one of the following types:

- Primitive types:
    - `null`
    - boolean: `true` or `false`
    - number (double-precision float)
    - string (currently ascii only)
    - C++ pointer
        - This doesn't do anything, but it's a way to pass values around the host program via Tack. Represented by `void*`. They can only be constructed by the host program
        ```rust
        let y = 1.23
        let x = "hello world"
        let z = (y == 1.23)           "// true "
        let w = null
        ```
- Compound types
    - array
        - A 0-indexed growable list
    - object
        - Lua table-style associative array. Keys must be strings (for now), values can be anything
        ```rust
        let x = []
        let y = [ 1, 2, "hello", {} ]
        let z = {}
        let w = {
            lang = "tack",
            version = "0.1"
        }
        ```

    For more details, see [Objects and arrays](OBJECTS_AND_ARRAYS.md)

- Function types
    - Function
        - A tack function
        - Can accept any number of arguments (up to the available number of registers, which should be 200+), can return one value. If nothing is returned, ie an empty `return` statement or no return statement at all, the caller receives `null`
        ```rust

        " named functions "
        fn func1(arg1, arg2) {
            return arg1 + arg2
        }
        fn factorial(n) {
            if (n <= 1) {
                return 1;
            }
            return n * factorial(n - 1)
        }

        " anonymous and temporary functions "
        let f = fn() {
            print("hello world")
        }
        const g = fn() {
            print("hello world 2")
        }
        fn(funcs) {
            for func in funcs {
                func()
            }
        }([ f, g ])
        ```
    - C++ function
        - See [Embedding](EMBEDDING.md)

##### "Truthiness"

Any value is implicitly convertible to boolean for use in if/while constructs, boolean operators,  predicate functions etc.

The following values implicitly convert to `false`

- `false` itself
- `null`
- the number `0.0`

All other values (including null pointers, empty strings, empty arrays and objects) will convert to `true`

##### Converting to string

Any value can be converted to a string representation with the standard library function `tostring`.

---

#### Modules

A module is defined in a `*.tack` file, and consists of a sequence of newline-separated statements

#### Statements

A statement is one of:

- Import statement
- Variable declaration
- Reassignment
- If construct
- While loop
- For loop
- Return statement
- Unbound expression: Standalone expressions are allowed as statements; even if the results are not bound to a variable, any side effects of the expression will still be observable.


##### Import statement

An import statement is of the form

```rust
import module_name
```
When this is encountered, and the module `module_name` has not previously been imported, the VM searches for a file with the name `module_name.tack` in all the configured module search directories. The module is then cached for future imports. Any `export`-ed variables from the module are now accessible in the current scope.

Imports can occur at any scope but are recommended to be placed at the top level of a file before any other statements.


##### Variable declaration
A variable is defined with one of the following:
    
```rust
let name = <exp>
const name = <exp>
fn name() {}

export let name = <exp>
export const name = <exp>
export fn name() {}
```

where `name` is a valid C-style identifier, and `exp` is any expression

The third form is equivalent to `const name = fn() {}`, but allows the function to refer to itself recursively by name.

The optional export keyword makes it possible to use the variable in other files, if the current file is imported - see `import` statement

##### Reserved keywords

These keywords may not be used as variable names:

    true false null import export let const fn if else while for return or and in


##### Reassignment

Reassignment can be done to non-const variables, or elements of compound values. The new type need not be the same as the old.

The LHS of the assignment must be an `identifier`, an indexing expression `<exp>[<exp>]` or an accessing expression `<exp>.ident`.

```rust
" Declarations "
let a = 1
let b = 2
let c = [ 1, 2, 3 ]
let d = { x = 1, y = 2 }
const f = fn() { return d }

" Reassignments "
a = 1
b = "string not number!"

c[2] = 4
c[3] = "error! out of range"

d.x = 100
f().y = 101

f = "error! f is const"

```


##### If and while loop:
The result of the `exp` is checked for truthiness. Parentheses are not required!

```rust
" if "
if <exp> {
    ...
}

" if-else "
if <exp> {
    ...
} else if <exp> {
    ...
} else if <exp> {
    ...
} else {
    ...
}

" while loop "

while <exp> {
    ...
}

```

Note that `break` or similar statement is not implemented.

##### For loop

There are several forms of for loop:

```rust

for i in <exp1>, <exp2> {
    " exp1 and exp2 must evaluate to number "
    " i loops through integers in the range exp1 <= i < exp2, incrementing by 1 each time"
}

for e in <exp> {
    " exp must evaluate to array or object "
    " if array, e takes on the elements of the array "
    " if object, e takes on the keys of the object (order is undefined)"
}

for k, v in <exp> {
    " exp must evaluate to object "
    " loop over key-value pairs of the object "
    " k and v take on the key and value in each iteration respectively "
}

```


##### Return statement

Inside a function, a return statement ceases execution of the function and returns to the caller with the return value.
In the file-level scope, the return statement ceases execution of the module.


##### Expressions

An expression can be one of

- A binary expression: `<exp> <op> <exp>`
- An unary expression: `<op> <exp>`
- Function call `func(a1, a2)`
- Index expression `array[number]` or `object["key"]`
- Identifier of a variable
- Primary expression: `( <exp> )` where exp is any expression
- Literal values: null, boolean, number, string, object, array, function all have literal syntax.

For a list of operators see [Operators](OPERATORS.md)