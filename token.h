// token.h

#ifndef _LANG_TOKEN_H
#define _LANG_TOKEN_H



enum TokenType
{
    Token_Unknown = 0,

    Token_Number,
    Token_String,
    Token_Symbol, // operator, semicolon, etc
    Token_Name, // identifier or keyword

    Token_LBrace,
    Token_RBrace,
    Token_LParen,
    Token_RParen,
    Token_LBracket,
    Token_RBracket,

    Token_Comment,
    Token_CommentMulti,
    Token_Whitespace,
    Token_Newline,
};

struct Token
{
    TokenType type;
    string val;
};


#define is_opchar(s) (string("+-/%*=!<>&|^~?:;").find(s) != string::npos)

TokenType decideToken(char c1, char c2)
{
    switch (c1)
    {
        case '/':
        {
            if (c2 == '/') return Token_Comment;
            if (c2 == '*') return Token_CommentMulti;
        }
        case '[': return Token_LBracket;
        case ']': return Token_RBracket;
        case '(': return Token_LParen;
        case ')': return Token_RParen;
        case '{': return Token_LBrace;
        case '}': return Token_RBrace;

        case '\'': 
        case '"': return Token_String;
        case ' ':
        case '\t':return Token_Whitespace;
        case '\n':return Token_Newline;
    }

    if (c1 <= '9' && c1 >= '0')
    {
        return Token_Number;
    }

    if ((c1 <= 'z' && c1 >= 'a') ||
        (c1 <= 'Z' && c1 >= 'A') ||
        (c1 == '_'))
    {
        return Token_Name;
    }

    if (is_opchar(c1))
    {
        return Token_Symbol;
    }

    return Token_Unknown;
}


bool acceptNumber(char c) { return c <= '9' && c >= '0'; }
bool acceptWhitespace(char c) { return c == ' ' || c == '\t'; }
bool acceptSymbol(char c) { return is_opchar(c); }
bool acceptName(char c) { return (c <= 'z' && c >= 'a') || (c <= 'Z' && c >= 'A') || (c <= '9' && c >= '0') || c == '_'; }
bool acceptComment(char c) { return (c != '\n'); }

template<bool(*accept)(char)>
string readTokenGeneric(const string& code)
{
    string val = "";
    int pos = 0;
    while (pos < code.size())
    {
        char c = code[pos];
        if (accept(c))
        {
            val += c;
        }
        else
        {
            break;
        }
        pos++;
    }
    return val;
}

string readTokenCommentMulti(const string& code)
{
    string val = "";
    int pos = 0;
    char first = code[0];

    while (pos < code.size())
    {
        char c = code[pos];

        if (c == '*' && code[pos+1] == '/')
        {
            val += "*/";
            pos += 2;
            return val;
        }

        val += c;
        pos++;
    }

    return val;
}
string readTokenString(const string& code)
{
    // this code is a bit insane
    // needs testing anyway
    string val = "";
    int pos = 0;
    char first = code[0];

    while (pos < code.size())
    {
        char c = code[pos];

        // TODO: need to reprocess strings for escape cahracters
        if (c == '\\' && code[pos+1] == first)
        {
            val += '\\';
            val += first;
            pos += 2;
            continue;
        }
        if (c == first && pos > 0)
        {
            val += first;
            break;
        }

        val += c;
        pos++;
    }

    if (val[0] != val[val.size()-1])
    {
        cout << "Invalid string token: " << "'" << val << "'" << endl;
        exit(0);
    }

    return val;
}
string readTokenWhitespace(const string& code)
{
    string val = "";
    int pos = 0;
    while (pos < code.size())
    {
        char c = code[pos];
        if (c == ' ' || c == '\t')
        {
            val += c;
        }
        else
        {
            break;
        }

        pos++;
    }
    return val;
}
template<char a>
string readTokenChar(const string& code)
{
    return (code[0] == a) ? string()+a : string();
}


string readToken(const string& code, TokenType type)
{
    switch (type)
    {
        case Token_String:          return readTokenString(code);
        case Token_CommentMulti:    return readTokenCommentMulti(code);

        case Token_Comment:         return readTokenGeneric<acceptComment>(code);
        case Token_Number:          return readTokenGeneric<acceptNumber>(code);
        case Token_Symbol:          return readTokenGeneric<acceptSymbol>(code);
        case Token_Name:            return readTokenGeneric<acceptName>(code);
        case Token_Whitespace:      return readTokenGeneric<acceptWhitespace>(code);

        case Token_Newline:         return readTokenChar<'\n'>(code);
        case Token_LBrace:          return readTokenChar<'{'>(code);
        case Token_RBrace:          return readTokenChar<'}'>(code);
        case Token_LParen:          return readTokenChar<'('>(code);
        case Token_RParen:          return readTokenChar<')'>(code);
        case Token_LBracket:        return readTokenChar<'['>(code);
        case Token_RBracket:        return readTokenChar<']'>(code);


        default: cout << "unknown token: " << code.substr(0, 10) << "..." << endl; exit(0); break;
    }

}

void writeToken(const Token& t)
{
    cout << "TOKEN: ";

    switch (t.type)
    {
        case Token_Number:       cout << "number       " << t.val << endl; break;
        case Token_String:       cout << "string       " << t.val.substr(0, 30) << "..." << endl; break;
        case Token_Symbol:       cout << "symbol       " << t.val << endl; break;
        case Token_Name:         cout << "name         " << t.val << endl; break;
    
        case Token_LBrace:       cout << "lbrace       " << endl; break;
        case Token_RBrace:       cout << "rbrace       " << endl; break;
        case Token_LParen:       cout << "lparen       " << endl; break;
        case Token_RParen:       cout << "rparen       " << endl; break;
        case Token_LBracket:     cout << "lbracket     " << endl; break;
        case Token_RBracket:     cout << "rbracket     " << endl; break;
    
        case Token_Comment:      cout << "comment      " << endl; break;
        case Token_CommentMulti: cout << "commentmulti " << endl; break;
        case Token_Whitespace:   cout << "whitespace   " << endl; break;
        case Token_Newline:      cout << "newline      " << endl; break;
        default:                 cout << "???"           << endl; break;
    }
}

vector<Token> tokenize(const string& src)
{
    vector<Token> tokens;
    string code = src;

    while (code.size())
    {
        Token token;
        token.type = decideToken(code[0], (code.size() > 1) ? code[1] : 0);
        if (token.type == Token_Unknown)
        {
            cout << "Unknown token" << endl;
            exit(0);
        }

        token.val = readToken(code, token.type);
        writeToken(token);

        code = code.substr(token.val.size(), src.size());
        tokens.push_back(token);
    }

    return tokens;
}

#endif