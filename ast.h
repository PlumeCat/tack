// ast.h

#ifndef _LANG_AST_H
#define _LANG_AST_H


#include "symbol.h"

typedef vector<Symbol>::const_iterator SymbolIter;

enum AstNodeType
{
    Ast_Unknown = 0,
    Ast_Program = 1,
    Ast_FuncDecl,
    Ast_VarDecl,

    Ast_Exp,
    Ast_ExpPostfix,
    Ast_ExpPrefix,

    Ast_ExpPrimary,

    Ast_Identifier,
    Ast_LiteralString,
    Ast_LiteralNumber,
};

struct AstNode
{
    AstNodeType type;
    string data;
    vector<AstNode*> children;
};

AstNode* parse_logic_binary_exp()
{
    
}

AstNode* parse_ternary_exp()
{
    /*
    expect(parse_operation && QUESION && parse_exp && )
     */
}

AstNode* parse_assign_exp()
{
    /*
    expect((IDENTIFIER && OP_ASSSIGN && parse_ternary_exp)) || parse_ternary_exp)
     */
}

AstNode* parse_exp()
{
    /*
    expect(parse_assign_exp))
     */
}

AstNode* parse_exp_statement()
{
    /*
    expect(parse_exp)
    expect(SEMICOLON)
     */
    
    return nullptr;
}

AstNode* parse_statement()
{
    /*
    expect(parse_if_construct ||
            parse_for_construct ||
            parse_while_construct ||
            parse_return_statement ||
            parse_break_statement ||
            parse_continue_statement ||
            parse_exp_statement)
     */
    
    return nullptr;
}

AstNode* parse_statement_block()
{
    /*
    expect(LBRACE)
    while option(parse_statement)
    expect(RBRACE)
     */
    
    return nullptr;
}

AstNode* parse_args_decl()
{
    /*
    expect(NAME)
    while option(COMMA && NAME)
     */
    
    return nullptr;
}

AstNode* parse_func_decl()
{
    /*
    expect(FUNC && NAME)
    expect(LPAREN)
    option(parse_args_decl)
    expect(RPAREN)
    expect(parse_statement_block)
     */
    
    return nullptr;
}

AstNode* parse_var_decl(const SymbolIter& start, const SymbolIter& end)
{
    /*
    expect(VAR && NAME)
    option(ASSIGN && EXPRESSION)
    expect(SEMICOLON)
     */
    
    return nullptr;
}

AstNode* parse_program(const SymbolIter& start, const SymbolIter& end)
{
    /*
    while not end
        expect(parse_func_decl || parse_var_decl)
     */

    return nullptr;
}

AstNode* parse_program(const vector<Symbol>& symbols)
{
    SymbolIter start = symbols.begin();
    SymbolIter end = symbols.end();

    AstNode* program = parse_program(start, end);
    if (!program)
    {
        cout << "error parsing program" << endl;
        exit(0);
    }
    return program;
}

#endif

/*
 */