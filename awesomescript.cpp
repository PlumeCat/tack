#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <time.h>
// #include <unistd.h>
#include <algorithm>

using namespace std;
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
const string opchars = string("=+-*/%!<>&|^~.?:");

#include "token.h"
#include "symbol.h"
#include "bnf.h"
#include "ast.h"



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
	}
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


int main()
{
	cout << "awesomescript" << endl;

	string code = readFile("source.txt");
	vector<Token> tokens;
	tokenize(code, tokens);

	vector<Symbol> symbols;
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

		symbols.push_back(tokenToSymbol(t));
	}

	// for (auto s : symbols)
	// {
	// 	cout << symbolToString(s) << endl;
	// }

	AstNode* scriptAst = parseScript(symbols);
	if (scriptAst)
	{
		cout << "successfully parsed script" << endl;
		printAst(scriptAst);
		delete scriptAst;
	}
	else
	{
		cout << "error parsing script" << endl;
	}

	return 0;
}