#ifndef _AST_H
#define _AST_H

typedef vector<Symbol>::iterator SymbolIter;

enum AstNodeType
{
	AST_SCRIPT = 1,

	AST_FUNC_DECL,
	AST_VAR_STATEMENT,
	AST_IMPORT_STATEMENT,

	AST_KEYWORD_IMPORT,
	AST_IDENTIFIER,
	AST_KEYWORD_VAR,

	AST_SEMICOLON,

	AST_OPERATOR_ASSIGN,
};
map<AstNodeType, string> astNodeNames = {
	{ AST_SCRIPT, 			"Script" },
	{ AST_FUNC_DECL, 		"FuncDecl" },
	{ AST_VAR_STATEMENT,	"VarStatement" },
	{ AST_IMPORT_STATEMENT, "ImportStatement" },
	{ AST_KEYWORD_IMPORT,	"Import" },
	{ AST_KEYWORD_VAR,		"Var" },
	{ AST_IDENTIFIER,		"Identifier" },
	{ AST_SEMICOLON,		"Semi" },
	{ AST_OPERATOR_ASSIGN,	"AssignOperator" }
};

string astNodeName(AstNodeType type)
{
	auto k = astNodeNames.find(type);
	if (k != astNodeNames.end())
	{
		return k->second;
	}

	return "<< invalid node type >>";
}

struct AstNode
{
	AstNodeType type;
	string data;
	vector<AstNode*> children;

	AstNode(AstNodeType t, string s="")
	{
		type = t;
		data = s;
	}
	~AstNode()
	{
		for (auto iter = children.begin(); iter != children.end(); iter++)
		{
			delete *iter;
		}
	}
};

AstNode* parseVarStatement(SymbolIter& symbol)
{
	if (symbol->type == SYMBOL_VAR &&
		(symbol+1)->type == SYMBOL_NAME)
	{
	}
	

	return nullptr;
}

AstNode* parseFuncDecl(SymbolIter& symbol)
{
	return nullptr;
}

AstNode* parseImportStatement(SymbolIter& symbol)
{
	if (symbol->type == SYMBOL_IMPORT &&
		(symbol+1)->type == SYMBOL_NAME &&
		(symbol+2)->type == SYMBOL_SEMICOLON)
	{
		AstNode* node = new AstNode(AST_IMPORT_STATEMENT);

		node->children.push_back(new AstNode(AST_KEYWORD_IMPORT));
		node->children.push_back(new AstNode(AST_IDENTIFIER, (symbol+1)->data));
		node->children.push_back(new AstNode(AST_SEMICOLON));

		symbol += 3;
		return node;
	}

	return nullptr;
}

AstNode* parseScript(vector<Symbol>& symbols)
{
	vector<Symbol>::iterator symbol = symbols.begin();

	AstNode* node = new AstNode(AST_SCRIPT);
	while (symbol != symbols.end())
	{
		AstNode* child = parseImportStatement(symbol);
		if (child)
		{
			node->children.push_back(child);
			continue;
		}

		child = parseVarStatement(symbol);
		if (child)
		{
			node->children.push_back(child);
			continue;
		}

		child = parseFuncDecl(symbol);
		if (child)
		{
			node->children.push_back(child);
			continue;
		}

		delete node;
		return nullptr;
	}

	return node;
}

void printAst(AstNode* node, string indent="")
{
	cout << indent << astNodeName(node->type) << ": " << node->data << "\n";
	for (int i = 0; i < node->children.size(); i++)
	{
		printAst(node->children[i], indent+"  ");
	}
}

#endif