// symbol.h
#ifndef _LANG_SYMBOL_H
#define _LANG_SYMBOL_H

enum SymbolType
{
    Symbol_Unknown = 0,

    Symbol_String,
    Symbol_Number,
    Symbol_Name,

    // symbols
    Symbol_Question,
    Symbol_Colon,
    Symbol_Semicolon,
    Symbol_LBrace,
    Symbol_RBrace,
    Symbol_LParen,
    Symbol_RParen,
    Symbol_LBracket,
    Symbol_RBracket,

    // keywords
    Symbol_KwIf,
    Symbol_KwElse,
    Symbol_KwFor,
    Symbol_KwWhile,

    Symbol_KwVar,
    Symbol_KwFunc,
    Symbol_KwObject,
    Symbol_KwArray,

    // operators
    Symbol_OpAssign,

    Symbol_OpAdd, // x2
    Symbol_OpSub, // x2
    Symbol_OpMul,
    Symbol_OpDiv,
    Symbol_OpMod,
    Symbol_OpPow,

    Symbol_OpAssignAdd,
    Symbol_OpAssignSub,
    Symbol_OpAssignMul,
    Symbol_OpAssignDiv,
    Symbol_OpAssignMod,
    Symbol_OpAssignPow,

    Symbol_OpAssignBitor,
    Symbol_OpAssignBitand,
    Symbol_OpAssignBitxor,
    Symbol_OpAssignBitLeft,
    Symbol_OpAssignBitRight,

    Symbol_OpIncPost,
    Symbol_OpDecPost,
    Symbol_OpIncPre,
    Symbol_OpDecPre,

    Symbol_OpCmpEq,
    Symbol_OpCmpNotEq,
    Symbol_OpCmpLt,
    Symbol_OpCmpLtEq,
    Symbol_OpCmpGt,
    Symbol_OpCmpGtEq,

    Symbol_OpLogicNot,
    Symbol_OpLogicAnd,
    Symbol_OpLogicOr,
    Symbol_OpLogicXor,

    Symbol_OpBitNot,
    Symbol_OpBitAnd,
    Symbol_OpBitOr,
    Symbol_OpBitXor,
    Symbol_OpBitLeft,
    Symbol_OpBitRight,
};

struct Symbol
{
    SymbolType type;
    string val;
};

// get Symbol_Name or Symbol_Kw* for some token of type Token_Name
SymbolType get_name(const string& token)
{
    return Symbol_Unknown;
}

// get Symbol_Op* for some token of type Token_Symbol
SymbolType get_symbol(const string& token)
{
    return Symbol_Unknown;
}

Symbol token_to_symbol(const Token& t)
{
    Symbol s;

    switch (t.type)
    {
    case Token_Unknown:
    case Token_Comment:
    case Token_CommentMulti:
    case Token_Whitespace:
    case Token_Newline:
        s.type = Symbol_Unknown;
        break;

    case Token_Number:
        s.type = Symbol_Number;
        s.val = t.val;
        break;
    case Token_String:
        s.type = Symbol_String;
        s.val = t.val;
        break;
        break;
    case Token_Name:
        s.type = get_name(t.val);
        break;

    case Token_Symbol:
        s.type = get_symbol(t.val);
        break;
    case Token_LBrace:
        s.type = Symbol_LBrace;
        break;
    case Token_RBrace:
        s.type = Symbol_RBrace;
        break;
    case Token_LParen:
        s.type = Symbol_LParen;
        break;
    case Token_RParen:
        s.type = Symbol_RParen;
        break;
    case Token_LBracket:
        s.type = Symbol_LBracket;
        break;
    case Token_RBracket:
        s.type = Symbol_RBracket;
        break;
    }

    return s;
}


vector<Symbol> symbolize(const vector<Token>& tokens)
{
    vector<Symbol> symbols;

    for (auto i = 0; i < tokens.size(); i++)
    {
        Symbol s = token_to_symbol(tokens[i]);
        if (s.type == Symbol_Unknown)
        {
            // cout << "Unknown symbol: " << tokens[i].val << endl;
            // exit(1);
            continue;
        }

        symbols.push_back(s);
    }

    return symbols;
}

#endif