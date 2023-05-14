# Arrays
Arrays resemble Javascript arrays and behave in the manner e

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

Object 

Once an object is created, new keys can be added with assignment syntax, but keys cannot be removed, and attempting to read from a key that is not present will result in an error

```rust
let x = {}
x.a = 1
x.b = 2
print(x.c) " error! "
```


## Iteration
To iterate over an object:

```rust
let obj = { x = 1, y = 2 }
for k, v in obj {
    print(x, y)
}
```
prints:
```
x = 1
y = 2
```

The single-variable form of `for in` also works, but as objects can't currently be dynamically indexed, this is of limited use


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
