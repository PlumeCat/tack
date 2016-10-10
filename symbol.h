#ifndef _SYMBOL_H
#define _SYMBOL_H


enum SymbolType
{
	SYMBOL_IMPORT = 1,
	SYMBOL_FUNCTION,
	SYMBOL_VAR,
	
	SYMBOL_LPAREN,
	SYMBOL_RPAREN,
	SYMBOL_LBRACE,
	SYMBOL_RBRACE, 
	SYMBOL_LBRACKET, 
	SYMBOL_RBRACKET,
	SYMBOL_COMMA,
	SYMBOL_SEMICOLON,
	SYMBOL_DOT,
	SYMBOL_QUESTION,
	SYMBOL_COLON,
	
	SYMBOL_RETURN,
	SYMBOL_CONTINUE,
	SYMBOL_BREAK,
	SYMBOL_IF,
	SYMBOL_ELSE,
	SYMBOL_FOR,
	SYMBOL_WHILE,

	SYMBOL_OP_ASSIGN,
	
	SYMBOL_OP_ADD_EQ,
	SYMBOL_OP_SUB_EQ,
	SYMBOL_OP_MUL_EQ,
	SYMBOL_OP_DIV_EQ,
	SYMBOL_OP_MOD_EQ,
	SYMBOL_OP_AND_EQ,
	SYMBOL_OP_OR_EQ,
	SYMBOL_OP_XOR_EQ,
	SYMBOL_OP_LSHIFT_EQ,
	SYMBOL_OP_RSHIFT_EQ,
	
	SYMBOL_OP_BOOLOR,
	SYMBOL_OP_BOOLAND,
	SYMBOL_OP_BITOR,
	SYMBOL_OP_BITAND,
	SYMBOL_OP_BITXOR,
	SYMBOL_OP_LSHIFT,
	SYMBOL_OP_RSHIFT,
	
	SYMBOL_OP_ADD,
	SYMBOL_OP_SUB,
	SYMBOL_OP_MUL,
	SYMBOL_OP_DIV,
	SYMBOL_OP_MOD,
	
	SYMBOL_OP_INCREMENT,
	SYMBOL_OP_DECREMENT,
	SYMBOL_OP_BITNOT,
	SYMBOL_OP_BOOLNOT,

	SYMBOL_OP_EQUAL,
	SYMBOL_OP_NOTEQUAL,
	SYMBOL_OP_LESS,
	SYMBOL_OP_MORE,
	SYMBOL_OP_LESS_EQ,
	SYMBOL_OP_MORE_EQ,

	SYMBOL_NAME,
	SYMBOL_STRING,
	SYMBOL_NUMBER,
};
struct Symbol
{
	SymbolType type;
	string data; // identifier, num-const, string-const
};

string symbolName(SymbolType type)
{
	switch (type)
	{
		case SYMBOL_IMPORT: return "IMPORT";
		case SYMBOL_FUNCTION:	return "FUNCTION";
		case SYMBOL_VAR:	return "VAR";
		case SYMBOL_LPAREN:	return "LPAREN";
		case SYMBOL_RPAREN:	return "RPAREN";
		case SYMBOL_LBRACE:	return "LBRACE";
		case SYMBOL_RBRACE:	return "RBRACE";
		case SYMBOL_LBRACKET:	return "LBRACKET";
		case SYMBOL_RBRACKET:	return "RBRACKET";
		case SYMBOL_COMMA:	return "COMMA";
		case SYMBOL_SEMICOLON:	return "SEMICOLON";
		case SYMBOL_DOT:	return "DOT";
		case SYMBOL_QUESTION:	return "QUESTION";
		case SYMBOL_COLON:	return "COLON";
		case SYMBOL_RETURN:	return "RETURN";
		case SYMBOL_CONTINUE:	return "CONTINUE";
		case SYMBOL_BREAK:	return "BREAK";
		case SYMBOL_IF:	return "IF";
		case SYMBOL_ELSE:	return "ELSE";
		case SYMBOL_FOR:	return "FOR";
		case SYMBOL_WHILE:	return "WHILE";
		case SYMBOL_OP_ASSIGN:	return "OP_ASSIGN";
		case SYMBOL_OP_ADD_EQ:	return "OP_ADD_EQ";
		case SYMBOL_OP_SUB_EQ:	return "OP_SUB_EQ";
		case SYMBOL_OP_MUL_EQ:	return "OP_MUL_EQ";
		case SYMBOL_OP_DIV_EQ:	return "OP_DIV_EQ";
		case SYMBOL_OP_MOD_EQ:	return "OP_MOD_EQ";
		case SYMBOL_OP_AND_EQ:	return "OP_AND_EQ";
		case SYMBOL_OP_OR_EQ:	return "OP_OR_EQ";
		case SYMBOL_OP_XOR_EQ:	return "OP_XOR_EQ";
		case SYMBOL_OP_LSHIFT_EQ:	return "OP_LSHIFT_EQ";
		case SYMBOL_OP_RSHIFT_EQ:	return "OP_RSHIFT_EQ";
		case SYMBOL_OP_BOOLOR:	return "OP_BOOLOR";
		case SYMBOL_OP_BOOLAND:	return "OP_BOOLAND";
		case SYMBOL_OP_BITOR:	return "OP_BITOR";
		case SYMBOL_OP_BITAND:	return "OP_BITAND";
		case SYMBOL_OP_BITXOR:	return "OP_BITXOR";
		case SYMBOL_OP_LSHIFT:	return "OP_LSHIFT";
		case SYMBOL_OP_RSHIFT:	return "OP_RSHIFT";
		case SYMBOL_OP_ADD:	return "OP_ADD";
		case SYMBOL_OP_SUB:	return "OP_SUB";
		case SYMBOL_OP_MUL:	return "OP_MUL";
		case SYMBOL_OP_DIV:	return "OP_DIV";
		case SYMBOL_OP_MOD:	return "OP_MOD";
		case SYMBOL_OP_INCREMENT:	return "OP_INCREMENT";
		case SYMBOL_OP_DECREMENT:	return "OP_DECREMENT";
		case SYMBOL_OP_BITNOT:	return "OP_BITNOT";
		case SYMBOL_OP_BOOLNOT:	return "OP_BOOLNOT";
		case SYMBOL_OP_EQUAL:	return "OP_EQUAL";
		case SYMBOL_OP_NOTEQUAL:	return "OP_NOTEQUAL";
		case SYMBOL_OP_LESS:	return "OP_LESS";
		case SYMBOL_OP_MORE:	return "OP_MORE";
		case SYMBOL_OP_LESS_EQ:	return "OP_LESS_EQ";
		case SYMBOL_OP_MORE_EQ:	return "OP_MORE_EQ";
		case SYMBOL_NAME:	return "NAME";
		case SYMBOL_STRING:	return "STRING";
		case SYMBOL_NUMBER:	return "NUMBER";
	}
}
string symbolToString(Symbol& s)
{
	string name = symbolName(s.type);
	int count = 20 - name.size();
	string c1 = name;
	c1.append(count, ' ');
	if (s.type == SYMBOL_NAME || s.type == SYMBOL_NUMBER || s.type == SYMBOL_STRING)
	{
		c1 += s.data;
	}
	return c1;
}
Symbol operatorToSymbol(Token& t)
{
	Symbol s;
	if (t.value == "=") { s.type = SYMBOL_OP_ASSIGN; return s; }
	
	if (t.value == "+=") { s.type = SYMBOL_OP_ADD_EQ; return s; }
	if (t.value == "-=") { s.type = SYMBOL_OP_SUB_EQ; return s; }
	if (t.value == "*=") { s.type = SYMBOL_OP_MUL_EQ; return s; }
	if (t.value == "/=") { s.type = SYMBOL_OP_DIV_EQ; return s; }
	if (t.value == "%=") { s.type = SYMBOL_OP_MOD_EQ; return s; }
	if (t.value == "&=") { s.type = SYMBOL_OP_AND_EQ; return s; }
	if (t.value == "|=") { s.type = SYMBOL_OP_OR_EQ; return s; }
	if (t.value == "^=") { s.type = SYMBOL_OP_XOR_EQ; return s; }
	if (t.value == "<<=") { s.type = SYMBOL_OP_LSHIFT_EQ; return s; }
	if (t.value == ">>=") { s.type = SYMBOL_OP_RSHIFT_EQ; return s; }
	
	if (t.value == "||") { s.type = SYMBOL_OP_BOOLOR; return s; }
	if (t.value == "&&") { s.type = SYMBOL_OP_BOOLAND; return s; }
	if (t.value == "|") { s.type = SYMBOL_OP_BITOR; return s; }
	if (t.value == "&") { s.type = SYMBOL_OP_BITAND; return s; }
	if (t.value == "^") { s.type = SYMBOL_OP_BITXOR; return s; }
	if (t.value == "<<") { s.type = SYMBOL_OP_LSHIFT; return s; }
	if (t.value == ">>") { s.type = SYMBOL_OP_RSHIFT; return s; }
	
	if (t.value == "+") { s.type = SYMBOL_OP_ADD; return s; }
	if (t.value == "-") { s.type = SYMBOL_OP_SUB; return s; }
	if (t.value == "*") { s.type = SYMBOL_OP_MUL; return s; }
	if (t.value == "/") { s.type = SYMBOL_OP_DIV; return s; }
	if (t.value == "%") { s.type = SYMBOL_OP_MOD; return s; }
	
	if (t.value == "++") { s.type = SYMBOL_OP_INCREMENT; return s; }
	if (t.value == "--") { s.type = SYMBOL_OP_DECREMENT; return s; }
	if (t.value == "~") { s.type = SYMBOL_OP_BITNOT; return s; }
	if (t.value == "!") { s.type = SYMBOL_OP_BOOLNOT; return s; }

	if (t.value == "==") { s.type = SYMBOL_OP_EQUAL; return s; }
	if (t.value == "!=") { s.type = SYMBOL_OP_NOTEQUAL; return s; }
	if (t.value == "<") { s.type = SYMBOL_OP_LESS; return s; }
	if (t.value == ">") { s.type = SYMBOL_OP_MORE; return s; }
	if (t.value == "<=") { s.type = SYMBOL_OP_LESS_EQ; return s; }
	if (t.value == ">=") { s.type = SYMBOL_OP_MORE_EQ; return s; }

	cout << "    ERROR: Unknown operator symbol: " << t.value << endl;
	exit(-6);
}
Symbol nameToSymbol(Token& t)
{
	Symbol s;
	if (t.value == "import") { s.type = SYMBOL_IMPORT; return s; }
	if (t.value == "function") { s.type = SYMBOL_FUNCTION; return s; }
	if (t.value == "var") { s.type = SYMBOL_VAR; return s; }
	if (t.value == "if") { s.type = SYMBOL_IF; return s; }
	if (t.value == "else") { s.type = SYMBOL_ELSE; return s; }
	if (t.value == "for") { s.type = SYMBOL_FOR; return s; }
	if (t.value == "while") { s.type = SYMBOL_WHILE; return s; }
	if (t.value == "continue") { s.type = SYMBOL_CONTINUE; return s; }
	if (t.value == "break") { s.type = SYMBOL_BREAK; return s; }
	if (t.value == "return") { s.type = SYMBOL_RETURN; return s; }

	s.type = SYMBOL_NAME; 
	s.data = t.value;
	return s;
}

Symbol tokenToSymbol(Token& t)
{
	Symbol s = Symbol();

	switch (t.type)
	{
		case TOKEN_NAME:
			return nameToSymbol(t);
		case TOKEN_NUMBER:
			s.data = t.value;
			s.type = SYMBOL_NUMBER;
			return s;
		case TOKEN_STRING:
			s.data = t.value;
			s.type = SYMBOL_STRING;
			return s;
		case TOKEN_OPERATOR:
			s = operatorToSymbol(t);
			return s;

		case TOKEN_SEMICOLON:s.type = SYMBOL_SEMICOLON; return s;
		case TOKEN_COMMA:    s.type = SYMBOL_COMMA; 	return s;
		case TOKEN_LBRACE:   s.type = SYMBOL_LBRACE; 	return s;
		case TOKEN_RBRACE: 	 s.type = SYMBOL_RBRACE; 	return s;
		case TOKEN_LBRACKET: s.type = SYMBOL_LBRACKET; 	return s;
		case TOKEN_RBRACKET: s.type = SYMBOL_RBRACKET; 	return s;
		case TOKEN_LPAREN:	 s.type = SYMBOL_LPAREN;	return s;
		case TOKEN_RPAREN:   s.type = SYMBOL_RPAREN;	return s;

		case TOKEN_UNKNOWN:
		case TOKEN_WHITESPACE:
		case TOKEN_COMMENT:
		case TOKEN_COMMENT_MULTI:
		default:
			cout << "\n" << "    ERROR: invalid token '" << t.value << "' : " << tokenName(t.type) << " passed to -symbolizer-" << "\n" << endl; 
			exit(-4);
	}
	return s;
}


#endif