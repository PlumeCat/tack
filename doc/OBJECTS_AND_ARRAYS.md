# Arrays

Arrays resemble Javascript arrays or Python lists, and behave in the same manner
Indexing starts at 0 and ends at N-1 where N is the length of the array

Arrays are created with the syntax

```rust
[ element1, element2, element3, ... ]
```

where an element can be any arbitrary expression.

Once an array is created, elements can be pushed/popped to the end with << and >> operators.

```rust

let a = []
a << 1
a << 2
a << 3

let c = a >> 1
print(c)      " 3 "
```

In addition there are several standard library functions for manipulating the contents of an array, or for computing new ones from existing values: [Standard library ](STANDARD_LIBRARY.md)

##### To iterate over an array:

```rust
array = [ 1, 2, 3, 4, 5 ]
" per-element iteration "
for x in array {
    print(x)
}
" numerical iteration "
for i in 0, #array {
    print(i, "=", array[i])
}
```
will output
```
1
2
3
4
5
0 = 1
1 = 2
2 = 3
3 = 4
4 = 5
```

Use the `#` operator to find the length of the array. This is O(1)

Use the `in` operator to check whether an element is present in the array. This is O(N)

# Objects

Objects resemble Javascript objects or Lua tables (the hash part), however not all the same features are yet present (and not all are planned).

Objects are created with the syntax 
```rust
{
    key1 = 1.0,
    key2 = true,
    key3 = fn () { ... }
    ...
}
```
Presently, keys must be valid identifiers. Values can be anything.

Once an object is created, new keys can be added with assignment syntax, but keys cannot be removed, and attempting to read from a key that is not present will result in an error

```rust
let x = {}
x.a = 1
x.b = 2
print(x.c) " error! "
```

Use the `#` operator to check the number of keys in the object. This is O(1)

Use the `in` operator to check whether a certain key can be found in the object. This is O(1), more or less

##### To iterate over an object:

```rust
let obj = { x = 1, y = 2 }
for k, v in obj {
    print(x, "=", y)
}
```
will output
```
x = 1
y = 2
```

The single-variable form of `for in` also works.


## Object-oriented programming

Similarly to Javascript pre-ES6, a rudimentary version of classes/object-orientation can be implemented with closures:

```rust
fn Rectangle(w, h) {
    let self = {
        width = w,
        height = h
    }

    self.area = fn() {
        return self.width * self.height
    }
    self.is_square = fn() {
        return self.width == self.height
    }
    
    return self
}

let r1 = Rectangle(4, 5)
let r2 = Rectangle(6, 6)
print(r1.area(), r2.area())
print(r1.is_square(), r2.is_square())
```
