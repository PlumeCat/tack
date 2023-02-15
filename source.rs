let primes_found = 1
let n = 1
while primes_found < 10000 {
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
        primes_found = primes_found + 1
    }
}


print(n)
