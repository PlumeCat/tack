"
let x = fn() {}

let primes_found = 1
let n = 1
let q = 0

let y = fn() {
    print(n)
} ""

while primes_found < 10 {
    n = n + 2

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
    }
}

print(n)
"
let x = 1
let y = 2

let maxrec = 0

let w = fn() {
    let a = 10
    let b = 20

    print(a + b)

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
print(1)