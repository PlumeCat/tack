#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <algorithm>

using namespace std;
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
const string opchars = string("=+-*/%!<>&|^~.?:");

enum TokenType
{
	TOKEN_UNKNOWN = 0,
	TOKEN_WHITESPACE = 1,
	TOKEN_NAME,
	TOKEN_SEMICOLON,
	TOKEN_COMMA,
	TOKEN_OPERATOR,

	TOKEN_NUMBER,
	TOKEN_STRING,

	TOKEN_LPAREN, TOKEN_RPAREN,
	TOKEN_LBRACE, TOKEN_RBRACE,
	TOKEN_LBRACKET, TOKEN_RBRACKET,

	TOKEN_COMMENT,
	TOKEN_COMMENT_MULTI,
};
struct Token
{
	string value;
	TokenType type;
};
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

string tokenName(TokenType type)
{
	switch (type)
	{
		case TOKEN_WHITESPACE: 	return "WHITESPACE";
		case TOKEN_NAME: 		return "NAME";
		case TOKEN_SEMICOLON: 	return "SEMICOLON";
		case TOKEN_COMMA:		return "COMMA";
		case TOKEN_OPERATOR: 	return "OPERATOR";
		case TOKEN_NUMBER: 		return "NUMERIC";
		case TOKEN_STRING: 		return "STRING";
		case TOKEN_LPAREN: 		return "PAREN_L";
		case TOKEN_RPAREN:		return "PAREN_R";
		case TOKEN_LBRACE:		return "BRACE_L";
		case TOKEN_RBRACE:		return "BRACE_R";
		case TOKEN_LBRACKET:	return "BRACKET_L";
		case TOKEN_RBRACKET:	return "BRACKET_R";
		case TOKEN_COMMENT:		return "COMMENT";
		case TOKEN_COMMENT_MULTI:return "COMMENT_MULTI";

		case TOKEN_UNKNOWN:
		default:
			return "UNKNOWN";
	}
}
TokenType tokenStart(const char* code)
{
	char c = code[0];
	switch (c)
	{
		case '{':	return TOKEN_LBRACE;
		case '}':	return TOKEN_RBRACE;
		case '[':	return TOKEN_LBRACKET;
		case ']':	return TOKEN_RBRACKET;
		case '(':	return TOKEN_LPAREN;
		case ')':	return TOKEN_RPAREN;
		case ';':	return TOKEN_SEMICOLON;
		case ',':	return TOKEN_COMMA;
	}
	if (c == '/')
	{
		if (code[1] == '*') return TOKEN_COMMENT_MULTI;
		if (code[1] == '/') return TOKEN_COMMENT;
	}
	if (c == '\'' || c == '"') return TOKEN_STRING;
	if (c <= '9' && c >= '0') return TOKEN_NUMBER;
	if ((c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A') || c == '_') return TOKEN_NAME;
	if (opchars.find(c) != string::npos) return TOKEN_OPERATOR;
	if (c == ' ' || c == '\t' || c == '\n') return TOKEN_WHITESPACE;
	return TOKEN_UNKNOWN;
}
int consumeWhitespace(char* str)
{
	char* ptr = str;
	while (true)
	{
		char c = *ptr;
		if (c == ' ' || c == '\t' || c == '\n')
		{
			ptr++;
		}
		else
		{
			break;
		}
	}

	return ptr - str; // hmm
}
int consumeName(char* str)
{
	char* ptr = str;
	while (true)
	{
		char c = *ptr;
		if ((c >= 'a' && c <= 'z') || 
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			(c == '_'))
		{
			ptr++;
		}
		else
		{
			break;
		}
	}

	return ptr - str;
}
int consumeOperator(char* str)
{
	char* ptr = str;
	while (true)
	{
		char c = *ptr;
		if (opchars.find(c) != string::npos)
		{
			ptr++;
		}
		else
		{
			break;
		}
	}

	return ptr - str;
}
int consumeNumber(char* str)
{
	char* ptr = str;

	while (true)
	{
		char c = *ptr;
		if (c <= '9' && c >= '0')
		{
			ptr++;
		}
		else
		{
			break;
		}
	}

	return ptr - str;
}
int consumeString(char* str)
{
	char start = str[0];
	char* ptr = str;
	ptr++;
	char p = start;

	while (true)
	{
		char c = *ptr;
		if (c == start && p != '\\')
		{
			ptr++;
			break;
		}
		else
		{
			ptr++;
			p = c;
		}
	}

	return ptr - str;
}
int consumeComment(char* str)
{
	char* ptr = str;
	while (true)
	{
		char c = *ptr;
		if (c == '\n')
		{
			ptr++;
			break;
		}
		else
		{
			ptr++;
		}
	}

	return ptr - str;
}
int consumeCommentMulti(char* str)
{
	char p = 'a';
	char* ptr = str;
	ptr++;

	while (true)
	{
		char c = *ptr;
		if (c == '/' && p == '*')
		{
			ptr++;
			break;
		}
		else
		{
			p = c;
			ptr++;
		}
	}

	return ptr - str;
}
int consumeToken(char* str, int len, Token& out)
{
	out.type = tokenStart(str);

	switch (out.type)
	{
		case TOKEN_WHITESPACE:
			return consumeWhitespace(str);
		case TOKEN_COMMENT:
			return consumeComment(str);
		case TOKEN_COMMENT_MULTI:
			return consumeCommentMulti(str);
		case TOKEN_STRING:
			return consumeString(str);
		case TOKEN_NUMBER:
			return consumeNumber(str);
		case TOKEN_NAME:
			return consumeName(str);
		case TOKEN_OPERATOR:
			return consumeOperator(str);
		case TOKEN_SEMICOLON:
		case TOKEN_COMMA:
		case TOKEN_LPAREN:
		case TOKEN_RPAREN:
		case TOKEN_LBRACE:
		case TOKEN_RBRACE:
		case TOKEN_LBRACKET:
		case TOKEN_RBRACKET:
			return 1;
		case TOKEN_UNKNOWN:
		default:
			return -1;
	}
}
void tokenize(const string& src, vector<Token>& tokens)
{
	string code = src;

	int i = 0;
	while (i < code.size())
	{
		usleep(1000);//safety

		Token t;
		int count = consumeToken(&code[i], src.size() - i, t);
		if (count < 0)
		{
			// error
			exit(7);
		}

		t.value = code.substr(i, count);
		i += count;
		tokens.push_back(t);
	}

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

	s.type = SYMBOL_NAME; return s;
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

// read the contents of a file in text mode
// if newlines, include the newline characters
string readFile(const string& fname, bool newlines=true)
{
	string code = "";
	ifstream file(fname);
	if (file.is_open())
	{
		while (!file.eof())
		{
			string line;
			getline(file, line);
			if (newlines)
			{
				code += line + "\n";
			}
		}
	}
	return code;
}

struct BnfRule
{
	string name;
	vector<vector<string> > expressions;
};
struct Bnf
{
	std::vector<BnfRule> rules;
};

void split(const string& str, vector<string>& tokens, char c)
{
	int lp = 0;
	int p = 0;
	while (true)
	{
		p = str.find(c, lp);
		string sub = str.substr(lp, ((p == string::npos) ? str.size() : p) - lp);

		if (sub.size())
		{
			tokens.push_back(sub);
		}

		if (p == string::npos)
		{
			break;
		}

		lp = p+1;
		usleep(10000);
	}
}

void parseBnf(Bnf& bnf)
{
	ifstream file("bnf.txt");
	if (!file.is_open())
	{
		cout << "Error: could not find bnf.txt" << endl;
		exit(2);
	}


	vector<string> bnfTokens;
	while (!file.eof())
	{
		string line;
		getline(file, line);
		line.erase(remove(line.begin(), line.end(), '\t'), line.end());
		vector<string> lineTokens;
		split(line, lineTokens, ' ');
		for (int i = 0; i < lineTokens.size(); i++)
		{
			string s = lineTokens[i];
			bnfTokens.push_back(s);
		}
	}
	file.close();

	// process bnf tokens
	BnfRule curRule;
	int mode = 0; // 0 = name, 1 = expression

	for (int i = 0; i < bnfTokens.size(); i++) {
		string t = bnfTokens[i];
		if ((t.size() == 0) || t[0] == ' ' || t[0] == '\t' || t[0] == '\n')
		{
			continue;
		}

		if (t[0] == ':')
		{
			// start expressions
			curRule.expressions.push_back(vector<string>());
			mode = 1;
			continue;
		}
		if (t[0] == '|')
		{
			// end current expression
			curRule.expressions.push_back(vector<string>());
			continue;
		}
		if (t[0] == ';')
		{
			// end current rule
			mode = 0;
			// start a new rule
			bnf.rules.push_back(curRule);
			curRule = BnfRule();
			continue;
		}

		if (mode == 0)
		{
			curRule.name = t;
		}
		else if (mode == 1)
		{
			curRule.expressions.back().push_back(t);
		}
	}

}

int main()
{
	cout << "awesomescript" << endl;

	Bnf bnf;
	parseBnf(bnf);

	string code = readFile("source.txt");
	vector<Token> tokens;
	tokenize(code, tokens);

	for (auto t : tokens)
	{
		if (0 ||
			t.type == TOKEN_UNKNOWN || 
			t.type == TOKEN_WHITESPACE ||
			t.type == TOKEN_COMMENT || 
			t.type == TOKEN_COMMENT_MULTI
			)
		{
			continue;
		}

		auto len = t.value.size();
		auto offset = max(100 - len, 0);
		auto space = string("").insert(0, offset, ' ');

		cout << "sym: " << t.value << space << tokenName(t.type);
		Symbol s = tokenToSymbol(t);
		cout << " -> " << symbolName(s.type) << endl;

	}

	cout << "done" << endl;

	return 0;
}