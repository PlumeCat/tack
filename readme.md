## awesome scripting language

an experimental scripting language with manual memory management
aiming to be as convenient as python/js/lua but no gc and non-ridiculous syntax


keywords
    import function var 
    if else 
    for while continue break return
    // array map struct class ctor dtor new delete null
special characters
    () {} [] , . ;

operators
    assign.op.      =
    cmp.op.         == != < > <= >=
    ar.un.op.       +   -
    ar.bin.op.      +   -   *   /   %
    ar.ass.op.      +=  -=  *=  /=  %=
    bw.un.op.       ~
    bw.bin.op.      & | ^ << >>
    bw.ass.op.      &= |= ^= <<= >>=
    bool.un.op.     !
    bool.bin.op.    && ||

keywords
    
    import
    struct function var
    new delete
    if else
    for while
    continue break return

the language

- strings are special cases of array[number]
- variables (var) are scoped to the local surrounding {}
- if a variable or function is top-level in a script file, it is global
    + persistent to all functions in that script
    + NOT accessible outside the script unless marked `export`
- imported declarations are global in the script they are imported into