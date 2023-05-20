# Standard Library

---

#### General

- `print(x...)`
    - x: any, variadic
    - returns: null

    Prints the string representation of each argument, space separated, to stdout
- `random()`
    - returns: number

    returns a "random" integer between 0 and 65535 inclusive
- `clock()`
    - returns: number

    returns time since epoch in seconds, with up to 6 decimal points of precision depending on the system/compiler
- `gc_disable()`
    - returns: null

    Disable the garbage collector
- `gc_enable()`
    - returns: null

    Enable the garbage collector
- `tostring(x)`
    - x: any
    - returns: string

    returns a string representation of x.
    It can be assumed that `x == y` if and only if `tostring(x) == tostring(y)` (unless x,y are objects or arrays)
    Arrays and objects will also be expanded recursively, for example
        `tostring([ 1, [ 2, 3 ] ]) == "array [ 1, array [ 2, 3 ] ]"`
        so be aware that the result may be quite long!
- `type(x)`
    - x: any
    - returns: string
    
    One of:
        `"null"`
        `"boolean"`
        `"number"`
        `"string"`
        `"pointer"`
        `"object"`
        `"array"`
        `"cfunction"`
        `"function"`
    (If `"unknown"` is returned, there is probably a bug in the VM, please raise an issue and include the offending tack source code)

---

#### Array
- `any(a)`
    - a: array
    - returns: boolean

    true if any of the values in a are truthy, otherwise false
- `all(a)`
    - a: array
    - returns: boolean

    true if all the values in a are truthy, otherwise false
- `next(a, f)`
    - a: array
    - f: function/cfunction (1 argument)
    - returns: any

    Returns the first element x from a for which f(x) == true, else null
- `foreach(a, f)`
    - a: array
    - f: function/cfunction (1 argument)
    - returns: null

    Iterates over the array and calls f once for each element, passing the element as the argument
- `map(a, f)`
    - a: array
    - f: function/cfunction (1 argument)
    - returns: array

    Returns a new array containing the value of f(x) for each element x in a
- `filter(a, f)`
    - a: array
    - f: function/cfunction (1 argument)
    - returns: array

    Returns a new array containing all x in a for which f(x) == true
- `reduce(a, f, s_0)`
    - a: array
    - f: function/cfunction
    - s_0: any
    - returns: any

    Iterates over the array and calls f(x, s_n), where x is the current element of the array and s_n is the return value from the previous invocation of f (or s_0 for the first invocation). Returns the return value from the final invocation of f
    Example: `reduce([ 1, 2, 3 ], fn(x, s) { return x + s }, 0) == 6`
- `sort(a, cmp)`
    - a: array
    - cmp: (optional) function/cfunction
    - returns: array

    Sorts the array into a new array.
    If cmp is not provided, the elements of the array should either all be number or all be string; numbers will be sorted smallest-first, strings in alphabetical order.
    If cmp is provided, it is used as the comparison function, so it should accept 2 arguments which will be any 2 values from the array, and return boolean

- `sum(a)`
    - a: array
    - returns: number|string

    Sum the contents of the array. a should contain all numbers or all strings. If a contains strings, they are concatenated with no space between
- `push(a, x)`
    - a: array
    - x: any
    - returns: null

    Append x to the back of a
- `push_front(a, x)`
    - a: array
    - x: any
    - returns: null

    Prepend x to the front of a
- `pop(a)`
    - a: array
    - returns: any

    Remove the last element of a, reducing the length by 1
    Returns the removed element
- `pop_front(a)`
    - a: array
    - returns: any

    Remove the first element of a; any remaining elements are shifted up by one.
    Returns the removed element
- `insert(a, x, n)`
    - a: array
    - returns: null

    Inserts x into the array at position n, moving any subsequent elements to make space
    Errors if n > #a
- `remove(a, x, y)`
    - a: array
    - x: number
    - y: number (optional)
    - returns: null

    Removes all elements from a where the index is in the range [ x, y )
    If y is not provided, only the element at index x will be removed (as if y = x + 1)
- `remove_value(a, x)`
    - a: array
    - returns: null

    Removes all instances of x from the array
- `remove_if(a, f)`
    - a: array
    - f: function/cfunction
    - returns: null

    Removes all elements x from a where f(x) == true. Similar to filter(), but acting inplace
- `join(a, sep)`
    - a: array
    - sep: string
    - returns: string

    Get the string representation for each element in a, and concatenate them all together in a string, separated by `sep`

---

#### Misc
- `min(x...)`
    - x: number or array
    - returns: number

    If one argument is provided, it should be an array; the minimum value from the array is returned.
    If more than one argument, they should all be number; the minimum argument is returned
- `max(x...)`
    - x: number or array
    - returns: number

    If one argument is provided, it should be an array; the maximum value from the array is returned.
    If more than one argument, they should all be number; the maximum argument is returned
- `slice(s, a, b)`
    - s: string or array
    - a: number
    - b: number
    - returns: string or array

    Returns the substring or subarray of s, starting at index a and ending before index b
    
    b is capped to the length of s.
- `find(s, x)`
    - s: string or array
    - x: string or any
    - returns: number

    Returns the index of the first occurrence of s in x, or -1 if not found

---

#### String
- `chr(x)`
    - x: number
    - returns: string

    The inverse of ord()
    
    Returns a single-character string containing the ascii string for x
    
    Eg: `chr(42) == "*"`
- `ord(x)`
    - x: string
    - returns: number

    The inverse of chr()
    
    Returns the ascii value for the first character of x
    
    Eg: `ord("*") == 42`
- `tonumber(s)`
    - s: string
    - returns: number or null

    Converts as many leading characters as possible from s into a number, ignoring initial whitespace
    (See std::strtod)
    
    Eg: `tonumber("    123hello") == 123`
    
    Returns null if a conversion was not possible
- `split(s, sep)`
    - s: string
    - sep: (optional) string
    - returns: array

    Split the string s at each instance of sep, returning the parts of s in a new array
    If sep is not provided, s is split at any whitespace.
    
    sep must be nonempty
- `replace(s, x, y)`
    - s: string
    - x: string
    - y: string
    - returns: string

    Return a new string equal to s, but with all instances of x replaced with y
    
    x must be nonempty, but y may be empty
- `tolower(s)`
    - s: string
    - returns: string

    Return a new string equal to s, but with any A-Z characters replaced with their lowercase versions
- `toupper(s)`
    - s: string
    - returns: string

    Return a new string equal to s, but with any a-z characters replaced with their uppercase versions
- `isupper(s)`
    - s: string
    - returns: boolean

    true if ALL charaters in s are in the range A-Z, otherwise false
- `islower(s)`
    - s: string
    - returns: boolean

    true if ALL characters in s are in the range a-z, otherwise false
- `isdigit(s)`
    - s: string
    - returns: boolean

    true if ALL characters in s are in the range 0-9, otherwise false
- `isalnum(s)`
    - s: string
    - returns: boolean

    true if ALL characters in s are within one of 0-9, a-z, A-Z, otherwise false

---

#### Object
- `keys(x)`
    - x: object
    - returns: array

    Returns a new array containing all the keys of x as string. The order is unspecified
- `values(x)`
    - x: object
    - returns: array

    Returns a new array containing the values in x. The order is unspecified

---
#### Math

For brevity, assume all the following functions accept number for all arguments, and return number

- `const pi`
    Number containing the value of pi
- `radtodeg(x)`
    Convert x from radians to degrees
- `degtorad(x)`
    Convert x from degrees to radians
- `lerp(a, b, x)`
    Linearly interpolate between a and b with the proportion x
    Equivalent to `a + (b - a) * x`
- `clamp(x, a, b)`
    Clamp x to the range [ a, b ]
    Equivalent to min(b, max(x, a))
- `saturate(x)`
    Clamp x to the range [ 0, 1 ]
    Equivalent to clamp(x, 0, 1)  
- `atan2(x, y)`
    Returns the angle between the x axis and the 2d vector (x, y)
    **Note: Differs from cmath in that 'x' is passed as the first argument and 'y' second**



#### The following functions are provided from cmath:

Reference: https://en.cppreference.com/w/cpp/header/cmath

- `sin(x)`
- `cos(x)`
- `tan(x)`
- `asin(x)`
- `acos(x)`
- `atan(x)`
- `sinh(x)`
- `cosh(x)`
- `tanh(x)`
- `asinh(x)`
- `acosh(x)`
- `atanh(x)`
- `exp(x)`
- `exp2(x)`
- `sqrt(x)`
- `log(x)`
- `pow(x, y)`
- `log2(x)`
- `log10(x)`
- `floor(x)`
- `ceil(x)`
- `abs(x)`
- `round(x)`
- `fmod(x, y)`
