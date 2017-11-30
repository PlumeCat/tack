#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

#include "token.h"
#include "symbol.h"
#include "ast.h"

string read_file(const string& fname)
{
    string src;
    ifstream file(fname);
    while (!file.eof())
    {
        string line;
        getline(file, line);
        src += line + '\n';
    }
    return src;
}
vector<string> split(const string& s, char c)
{
    vector<string> parts;
    int prev = 0;
    int end = s.size();

    while (true)
    {
        int next = s.find(c, prev);
        parts.push_back(s.substr(prev, (next?next:end)-prev));
        if (next == -1) break;
        prev = next+1;
    }

    return parts;
}

int main()
{
    string source = read_file("source.txt");
    
    vector<Token> tokens = tokenize(source);
    vector<Symbol> symbols = symbolize(tokens);

    AstNode* ast = parse_program(symbols);

    return 0;
}