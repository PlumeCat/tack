# grammar

# module == source file, basically
module
    def-list EOF

# top level definitions: functions and structures
def-list
    struct-def def-list
    func-def def-list
struct-def
    'struct' '{' member-def-list '}'
member-def-list
    <empty>
    ident ( ',' ident )*
func-def
    'def' ident '(' params-def ')' block
params-def
    <empty>
    ident ( ',' ident )*

# Statements
block
    '{' stat-list '}'
stat-list
    statement stat-list
    <empty>
statement
    var-decl
    assignment
    if-stat
    print-stat
print-stat
    'print' expr ';'
var-decl
    'var' ident '=' expr ';'
assignment
    ident '=' expr ';'
if-stat
    'if' '(' expr ')' block
    'if' '(' expr ')' block 'else' if-stat
    'if' '(' expr ')' block 'else' block

# Expressions
expr
    or-expr
or-expr
    and-expr '||' or-expr
    and-expr
and-expr
    xor-expr '&&' and-expr
    xor-expr
xor-expr
    cmp-expr '^^' cmp-expr
    cmp-expr
cmp-expr
    add-expr '==' add-expr
    add-expr '!=' add-expr
    add-expr '<=' add-expr
    add-expr '>=' add-expr
    add-expr '<' add-expr
    add-expr '>' add-expr
add-expr
    sub-expr '+' add-expr
    sub-expr
sub-expr
    mod-expr '-' sub-expr
    mod-expr
mod-expr
    mul-expr '%' mul-expr
    mul-expr
mul-expr
    div-expr '*' mul-expr
    div-expr
div-expr
    pow-expr '/' div-expr
    pow-expr
pow-expr
    lsh-expr '**' pow-expr
    lsh-expr
lsh-expr
    rsh-expr '<<' lsh-expr
    rsh-expr
rsh-expr
    b-and-expr '>>' rsh-expr
    b-and-expr
b-and-expr
    b-or-expr '&' b-and-expr
    b-or-expr
b-or-expr
    b-xor-expr '|' b-or-expr
    b-xor-expr
b-xor-expr
    unary-add-expr '^' b-xor-expr
    unary-add-expr
unary-add-expr
    '+' unary-add-expr
    unary-sub-expr
unary-sub-expr
    '-' unary-sub-expr
    unary-not-expr
unary-not-expr
    '!' unary-not-expr
    unary-b-not-expr
unary-b-not-expr
    '~' unary-b-not-expr
    primary-expr
primary-expr
    call-expr # also starts with ident, be careful
    ident
    literal
    '(' expr ')'
call-expr
    ident '(' args-list ')'
args-list
    expr (',' expr)*
    <empty>
literal
    num-literal
    str-literal
