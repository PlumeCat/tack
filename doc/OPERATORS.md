# Operators


## Unary operators:
- `-, !, ~, #`

## Binary operators:
- `or, and`
- ` +, -, *, /, %, **`
- `<<, >>, ^, &, |`
- `<=, <, >=, >`

## Assignment operator:
- `=`

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