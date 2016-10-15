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

	AstNode(AstNodeType t, string s="", double d=0.0f)
	{
		type = t;
		strData = s;
		numData = d;
	}
};

AstNode* makeAst(vector<BnfRule>& bnf, vector<Symbol>::iterator startSymbol, vector<Symbol>::iterator endSymbol, AstNodeType type)
{
	AstNode* node = new AstNode(type);

	auto current = startSymbol;
	while (current < endSymbol)
	{
		for (int i = 0; i < bnf.size(); i++)
		{
		}
	}

	return node;
}

#endif