#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;

#define rule {
#define is ,{{
#define alt },{
#define end }}},

struct BnfRule
{
	string name;
	vector<vector<string>> expressions;
};

/*
ast 

script:
	[import | func-decl | var-statement]

func-decl:
	FUNCTION NAME LPAREN [NAME] RPAREN statement-block

statement-block LBRACE [statement] RBRACE

var-statement:
	VAR NAME
	VAR NAME OP-ASSIGN expression

statement:
	var-statement;
	expression SEMI;
	RETURN expression SEMI;
	BREAK SEMI;
	CONTINUE SEMI;
	conditional-construct
	for-loop
	while-loop

conditional-construct
	if-construct
	if-construct else-construct
	if-construct [elseif-construct] else-construct

if-construct:
	IF LPAREN expression RPAREN statement-block
elseif-construct:
	ELSE IF LPAREN expression RPAREN statement-block
else-construct:
	ELSE statement-block

for-loop:
	FOR LPAREN statement SEMI expression SEMI expression RPAREN statement-block

while-loop:
	WHILE LPAREN expression RPAREN statement-block


expression:
	...


*/

vector<BnfRule> bnf = {
	rule "script" 			is  "decl-list" end
	rule "decl-list" 		is  "decl-list", "decl"
							alt "decl" end
	rule "decl"				is  "func-decl" 
							alt "var-statement" 
							alt "import-statement" end
	rule "var-statement" 	is  "VAR", "NAME", "SEMI"
							alt "VAR", "NAME", "OP-ASSIGN", "expression", "SEMI" end
	rule "import-statement" is  "IMPORT", "NAME", "SEMI" end
	rule "func-decl"		is  "FUNCTION", "NAME", "LPAREN", "args-decl", "RPAREN", "statement-block" 
							alt "FUNCTION", "NAME", "LPAREN", "RPAREN", "statement-block" end
	rule "args-decl" 		is  "args-decl", "COMMA", "NAME" 
							alt "NAME" end
	rule "statement-block" 	is "LBRACE", "statement-list", "RBRACE" end
	rule "statement-list"  	is "statement-list", "statement" 
							alt "statement" end
	rule "statement"		is "expression", "SEMI"
							alt "return-statement"
							alt "break-statement"
							alt "continue-statement"
							alt "select-construct"
							alt "for-construct"
							alt "while-construct" end
	rule "return-statement" is "RETURN", "expression", "SEMI"
							alt "RETURN", "SEMI" end
	rule "break-statement"	is "BREAK", "SEMI" end
	rule "continue-statement" is "CONTINUE", "SEMI" end
	rule "select-construct" is "if-construct"
							alt "if-construct elseif-list"
							alt "if-construct else-construct"
							alt "if-construct elseif-list" end
 	rule "if-construct"		is "IF", "LPAREN", "expression", "RPAREN", "statement-block" end
 	rule "elseif-construct" is "ELSE", "IF", "LPAREN", "expression", "RPAREN", "statement-block" end
 	rule "else-construct" 	is "ELSE", "statement-block" end
 	rule "elseif-list"		is "elseif-list", "elseif-construct"
 							alt "elseif-construct" end
 	rule "while-construct"	is "WHILE", "LPAREN", "expression", "RPAREN", "statement-block" end
 	rule "for-construct"	is "FOR", "LPAREN", "expression", "SEMI", "expression", "SEMI", "expression", "RPAREN", "statement-block" end

 	rule "expression"		is "assign-exp" end
 	rule "assign-exp"		is "assign-rhs", "OP-EQUALS", "expression"
 							alt "assign-rhs", "OP-ADDEQ", "expression"
 							alt "assign-rhs", "OP-SUBEQ", "expression"
 							alt "assign-rhs", "OP-MULEQ", "expression"
 							alt "assign-rhs", "OP-DIVEQ", "expression"
 							alt "assign-rhs", "OP-MODEQ", "expression"
 							alt "assign-rhs", "OP-ANDEQ", "expression"
 							alt "assign-rhs", "OP-OREQ", "expression"
 							alt "assign-rhs", "OP-XOREQ", "expression"
 							alt "assign-rhs", "OP-LSHIFTEQ", "expression"
 							alt "assign-rhs", "OP-RSHIFTEQ", "expression"
 							alt "ternary-exp" end
 	rule "assign-rhs"		is "NAME", "array-access" // write
 							alt "NAME" end // write
 	rule "ternary-exp"		is "expression", "QUESTION", "expression", "COLON", "expression" 
 							alt "bool-exp" end
 	rule "bool-exp"			is "bool-exp", "OP-BOOLOR", "bool-exp"
 							alt "bool-exp", "OP-BOOLAND", "bool-exp"
 							alt "bit-exp" end
 	rule "bit-exp"			is "bit-exp", "OP-BITAND", "bit-exp"
 							alt "bit-exp", "OP-BITOR", "bit-exp"
 							alt "equality-exp" end
 	rule "compare-exp"		is "compare-exp", "OP-EQUALITY", "compare-exp",
 							// noteq, less, more, leseq, moreeq
 							alt "bitshift-exp" end
 	rule "bitshift-exp"		is "bitshift-exp", "OP-LSHIFT", "bitshift-exp"
 							alt "bitshift-exp", "OP-RSHIFT", "bitshift-exp"
 							alt "add-exp" end
 	rule "add-exp"			is "add-exp", "OP-ADD", "add-exp"
 							alt "add-exp", "OP-SUB", "add-exp"
 							alt "mul-exp" end
 	rule "mul-exp"			is "mul-exp", "OP-MUL", "mul-exp"
 							alt "mul-exp", "OP-DIV", "mul-exp"
 							alt "mul-exp", "OP-MOD", "mul-exp"
 							alt "prefix-exp" end
 	rule "prefix-exp"		is "OP-ADD", "prefix-exp"
 							alt "OP-SUB", "prefix-exp"
 							alt "OP-BOOLNOT", "prefix-exp"
 							alt "OP-BITNOT", "prefix-exp"
 							alt "OP-INCREMENT", "prefix-exp"
 							alt "OP-DECREMENT", "prefix-exp"
 							alt "postfix-exp" end
 	rule "postfix-exp"		is "postfix-exp", "OP-INCREMENT"
 							alt "postfix-exp", "OP-DECREMENT"
 							alt "postfix-exp", "DOT", "NAME"
 							alt "postfix-exp", "array-access"
 							alt "postfix-exp", "func-call"
 							alt "primary-exp" end
 	rule "primary-exp"		is "NAME" // read
 							alt "STRING" 
 							alt "NUMBER" 
 							alt "LPAREN", "expression", "RPAREN" end


 	rule "array-access"		is "LBRACKET", "expression", "RBRACKET" end
 	rule "func-call"		is "LPAREN", "args-list", "RPAREN" 
 							alt "LPAREN", "RPAREN" end
 	rule "args-list"		is "args-list", "COMMA", "expression"
 							alt "expression" end

};


#undef rule
#undef is
#undef alt
#undef end