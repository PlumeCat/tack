"import foo"
"import bar.baz as baz"
"
fn check_prime(n) {
    let is_prime = 1
    let test = 2
    while test * test <= n {
        if n % test == 0 {
            is_prime = 0
        }
        test = test + 1
    }
}

print(2)
let primes_found = 1
let n = 1
let q = 0
while primes_found < 100 {
    n = n + 2
    if check_prime(n) {
        primes_found = primes_found + 1
        print(n)
    }
}
"


"
type Foo {
    x = 0
    y = 123
    z = some_func()
}

type Bar : Foo {

}
"
let fac = fn(n, fac) {
    if n == 0 {
        return 1
    }
    return fac(n - 1, fac) * n
}

print(fac(7, fac))

