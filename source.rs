print(2)

let n = 4
while n < 10 {
    print(n)
    n = n + 1
}

let foo = fn(a, b, c) {
    let bar = fn(x, y, z) {
        let a = 1
        let b = 2
        let baz = fn() {
            let e = 1
            let f = 4
            print(e + f)
        }
        baz()
    }

    print(100)
    bar()
    print(101)
    bar()
}

foo()
foo()

print(300)

"print(2)"

"
let x = fn() {}

let primes_found = 1
let n = 1
let q = 0

let y = fn() {
    print(12346789)
} ""

while primes_found < 10 {
    n = n + 2
    print(n)

    let is_prime = 1
    let test = 2
    while test * test <= n {
        if n % test == 0 {
            is_prime = 0
        }
        test = test + 1
    }

    if is_prime {
        y()
        primes_found = primes_found + 1
        print(n)
    }
}

print(n)"

"

let x = 1
let y = 2

let maxrec = 0

let w = fn() {
    let a = 10
    let b = 20

    print(a + b)

    x = x + 1
    print(x)
    if x == 3 {
        return
    }

    if maxrec < 10 {
        maxrec = maxrec + 1
        a = a + 1
        w()
    }
}

let z = fn() {
    print(2)
    w()
}

print(x + y)
z()
print(1)"