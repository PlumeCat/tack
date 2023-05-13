# Syntax

The Tack syntax is a mixture of Rust and Javascript; here is a guide (omitting details such as whitespace)

```brainfuck
stat-list:          stat '\n' stat-list 

stat:               import-stat
                    var-decl-stat
                    assign-stat
                    if-stat
                    while-stat
                    for-stat
                    return-stat
                    exp

import-stat:        'import' ident

var-decl-stat:      'let' ident '=' exp
                    'const' ident '=' exp
                    'fn' ident '(' param-list ')' block
                    'fn' ident '(' ')' block

param-list:         ident ',' param-list
                    ident

assign-stat:        access-exp '=' exp
                    index-exp  '=' exp
                    ident '=' exp

if-stat:            'if' exp block else-stat
                    'if' exp block

else-stat:          'else' if-stat
                    'else' block

while-stat:         'while' exp block

for-stat:           'for' ident 'in' exp block
                    'for' ident 'in' exp, exp block
                    'for' ident, ident 'in' exp block

block:              '{' stat-list '}'
                    '{' '}'

return-stat:        'return' exp
                    'return'

exp:                exp binary-operator unary-exp
                    unary-exp

unary-exp:          unary-operator unary-exp
                    postfix-exp

postfix-exp:        call-exp
                    access-exp
                    index-exp
                    primary-exp

call-exp:           postfix-exp '(' arg-list ')'

access-exp:         postfix-exp '.' ident

index-exp:          postfix-exp '[' exp ']'

primary-exp:        literal
                    ident
                    '(' exp ')'

literal:            'null'
                    'true'
                    'false'
                    literal-number
                    literal-string
                    literal-object
                    literal-array
                    literal-function

literal-object:     '{' key-val-list '}'
                    '{' '}'

literal-array:      '[' arg-list ']'
                    '[' ']'

literal-function:   'fn' '(' param-def ')' block
                    'fn' '(' ')' block

key-val-list:       ident '=' exp ',' key-val-list
                    ident '=' exp

arg-list:           exp ',' arg-list
                    exp


ident:              // C-style ident: match `[_a-zA-Z]([_a-zA-Z0-9]*)`
literal-number:     // decimal, hex, ...
literal-string:     // C-style double quoted string
binary-operator:    // see Operators
unary-operator:     // see Operators

```

---

## Notes

- For a list of binary and unary prefix/postfix operators, see the "Operators" documentation

- In general, the RHS of a var-decl cannot refer to the variable name.
    The "fn" form of var-decl makes the following code snippet:
        
    `fn f() { ... }`

    equivalent to 

    `let f = fn() { ... }`

    with one small difference: `f` can be referenced in the function body in form (1), but not in form (2). This is to make recursive named functions possible.

    (In future, for easier OO-closures, an object literal _may be able to_ reference the constructed object instance within its own definition by a `self` or `this` keyword; this is not supported yet)