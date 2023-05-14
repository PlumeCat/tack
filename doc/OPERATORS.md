# Operators

All available operators

## Unary operators
Unary operators always precede binary
- `-` Numeric negation: number => number

- `!` Boolean not: any => boolean
Operates on the truthiness value of the input, so can accept any type

- `~` Bitwise not: number => number
The input is downcasted to 64 bit integer internally, the result is integer

- `#` Length operator
Accepts string, array or object; returns the length as a number (object: the number of keys)

## Binary operators
Binary operators are listed in ascending order of precedence (`**` has the highest precedence, `or` has the lowest)

- `or, and` boolean operations
    - any, any => boolean
    - operates on the truthiness of the operands, so operands can be any type

- `in`: check if the LHS can be found in the RHS
    - any, array => boolean
    - string, object => boolean

- `|, &, ^` bitwise operations
    - number, number => number
    - numbers are down-cast to 64 bit integer internally; the result will be integer

- `<=, <, >=, >`
    - number, number => boolean

- `==`, `!=`
    - any, any => boolean

- `<<, >>`
    - number,number => number; downcasts the operands to 64 bit integer internally before bitshifting; result is integer
    - array << any is possible; pushes any value to the back of the array
    - array >> any is possible; pops a single value from the back of the array returns the popped value or null

- ` +, -, *, /, %, **`
    - number, number => number
    - string + string is also possible, returns the concatenation
    - array + array is also possible, returns the concatenation

---

## Associativity
All operators are left-associative, so

`VALUE op VALUE op VALUE`

will always be equivalent to

`(VALUE op VALUE) op VALUE` 

(concerning the operators that are not associative like `+`, `*`, `&`)

---
## Notes

- The comparison operators don't chain by design.

- `+` can work with numbers, strings or arrays

- `<<` is number-number, but also accepts array as the LHS (push to the end of the array)

- (Experimental): `>>` is number-number, accepts array as the RHS (pop the last value from the array; the RHS is unused for now)