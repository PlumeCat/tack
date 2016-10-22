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
	ass.op. 		=
	cmp.op.			== != < > <= >=

	ar.un.op.		+ 	-
	ar.bin.op. 		+ 	- 	* 	/ 	%
	ar.ass.op.		+=	-=	*=	/=	%=
	
	bw.un.op.		~
	bw.bin.op.		& | ^ << >>
	bw.ass.op.		&= |= ^= <<= >>=
	
	bool.un.op.		!
	bool.bin.op.	&& ||
	
	no boolean ass-ops

keywords
	
	import
	struct function var
	new delete
	if else
	for while
	continue break return

the language

	strings are special cases of array[number]
	
	variables (var) are scoped to the deepest surrounding {}

	if a variable or function is top-level in a script file, it is global
		=> accessible to all functions in that script
		=> NOT accessible outside the script
	however, variables and functions in the global scope that are
	also marked `export` are "super-global", they are accessible
	to any other script that imports it.
		=> imported declarations are global in the script they are imported into
		=> later you will be able to type `import foo as bar` to create a
		=> global namespace `bar` in the importer script, containing all the
		=> imports from foo.

	struct are allocated on the stack

	class are allocated on the heap
	class are allocated manually - you must new and delete them

	no garbage collection for classes

	however there is a garbage collector as part of the standard lib

	also, people will be able to write garbage collectors in modules, so different GC for different purpose will be possible (hopefully with minimal performance impact) and people can write them in the language itself! or in c

	there will be a package manager and a registry. packages can be written in the language or c (not sure how to handle compilation of c modules...)


