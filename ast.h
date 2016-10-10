#ifndef _AST_H
#define _AST_H



enum AstNodeType
{
	AST_SCRIPT = 1,

	AST_FUNC_DECL,
	AST_VAR_STATEMENT,
	AST_IMPORT_STATEMENT,
};

struct AstNode
{
	AstNodeType type;
	string strData;
	double numData;
	AstNode* sibling;
	AstNode* child;
};

AstNode* makeAst(vector<BnfRule>& bnf, vector<Symbol>& symbols, AstNode* parent)
{
	AstNode* node = new AstNode;

	

	return node;
}

#endif