#ifndef _TOKEN_H
#define _TOKEN_H

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


#endif