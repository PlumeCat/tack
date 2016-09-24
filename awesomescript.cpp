#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>

using namespace std;
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define opchars string("=+-*/%!<>&|^~.?:")

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
TokenType tokenStart(const string& code)
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
bool tokenConsume(Token& token, char c1, char c2)
{
	switch (token.type)
	{
		case TOKEN_WHITESPACE: 	return c1 == ' ' || c1 == '\t' || c1 == '\n';
		case TOKEN_NAME:		return ((c1 <= 'z' && c1 >= 'a') || (c1 <= 'Z' && c1 >= 'A') || c1 == '_');
		case TOKEN_SEMICOLON:	return c1 == ';';
		case TOKEN_OPERATOR:	return opchars.find(c1) != string::npos;
		case TOKEN_NUMBER:		return (c1 <= '9' && c1 >= '0');
		case TOKEN_STRING:		return (c1 != token.value[0]);
		case TOKEN_COMMENT:		return c1 != '\n';
		case TOKEN_COMMENT_MULTI:return !(c1 == '*' && c2 == '/');
		case TOKEN_LPAREN:		return c1 == '(';
		case TOKEN_RPAREN:		return c1 == ')';
		case TOKEN_LBRACKET:	return c1 == '[';
		case TOKEN_RBRACKET:	return c1 == ']';
		case TOKEN_LBRACE:		return c1 == '{';
		case TOKEN_RBRACE:		return c1 == '}';
		case TOKEN_COMMA:		return c1 == ',';
	}
	return true;
}
void tokenize(const string& src, vector<Token>& tokens)
{
	string code = src;
	Token current;
	current.type = TOKEN_UNKNOWN;
	current.value = "";

	for (int i = 0; i < code.size(); i++)
	{
		char c1 = code[i];
		char c2 = (i < code.size()-1) ? code[i+1] : 0;

		if (current.type == TOKEN_UNKNOWN)
		{
			current.type = tokenStart(code.substr(i, code.size()));
			if (current.type == TOKEN_UNKNOWN)
			{
				cout << "Unknown symbol " << c1 << endl;
				exit(1);
			}
		}
		if (tokenConsume(current, c1, c2))
		{
			current.value.append(1, c1);
		}
		else
		{
			tokens.push_back(current);
			current.type = TOKEN_UNKNOWN;
			current.value = "";
			i--; // provably safe I think
		}
	}

	if (current.type != TOKEN_UNKNOWN)
	{
		tokens.push_back(current);
	}
}

string readFile(const string& fname)
{
	string code = "";
	ifstream file(fname);
	if (file.is_open())
	{
		while (!file.eof())
		{
			string line;
			getline(file, line);
			code += line + "\n";
		}
	}
	return code;
}

struct BnfRule
{
	string name;
	vector<vector<string> > rules;
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

void readBnf()
{
	ifstream file("bnf.txt");
	if (!file.is_open())
	{
		cout << "Error: could not find bnf.txt" << endl;
		exit(2);
	}

	string currentName = "";
	vector<string> currentRule;

	while (!file.eof())
	{
		string line;
		getline(file, line);

		if (line[0] == '\t')
		{
			if (line[1] == '\t')
			{
				// new rule
			}
			else
			{
			}
		}
		else
		{
		}
	}

	file.close();
}

int main()
{
	cout << "awesomescript" << endl;

	string test = "splits  this  into tokens   if     YOU CAN";
	cout << test << endl;
	vector<string> testTokens;
	split(test, testTokens, ' ');
	for (int i = 0; i < testTokens.size(); i++)
	{
		cout << "        token: '" << testTokens[i] << "'" << endl;
	}
	cout << "done" << endl;

	string code = readFile("source.txt");
	vector<Token> tokens;
	tokenize(code, tokens);

	cout << tokens.size() << " tokens generated" << endl;

	for (auto t : tokens)
	{
		if (t.type == TOKEN_UNKNOWN || t.type == TOKEN_WHITESPACE)
		{
			continue;
		}

		auto len = t.value.size();
		auto offset = max(30 - len, 0);
		auto space = string("").insert(0, offset, ' ');

		cout << "token: " << t.value << space << tokenName(t.type) << endl;
	}

	return 0;
}