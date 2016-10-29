#ifndef _AST_H
#define _AST_H

typedef vector<Symbol>::iterator SymbolIter;

enum AstNodeType
{
	AST_SCRIPT = 1,

	AST_FUNC_DECL,
	AST_VAR_STATEMENT,
	AST_IMPORT_STATEMENT,

	AST_IDENTIFIER,
	AST_EXPRESSION,

	AST_CONST_STRING,
	AST_CONST_NUMBER,
};



/*
AST Node definitions:

	// { x } 	repeat x (zero or more)
	// x | y 	x or y
	// [ x ]	x is optional

	Script
		{ ImportStatement | FuncDecl | VarStatement }

	FuncDecl
		Identifier, { ArgDef }, StatementBlock

	ArgDef
		Identifier, [ Expression ]

	VarStatement
		Identifier, [ Expression ]

	ImportStatement
		Identifier

	IfConstruct
		IfBlock, [ { ElseIfBlock } ], [ ElseBlock ]

	IfBlock
		Expression, StatementBlock

	ElseIfBlock
		Expression, StatementBlock

	ElseBlock
		StatementBlock

	WhileLoop
		Expression, StatementBlock

	StatementBlock
		{ 	
			IfConstruct | 
			WhileLoop | 
			BreakStatement | 
			ContinueStatement | 
			ReturnStatement | 
			VarStatement | 
			ExpressionStatement 
		}

	BreakStatement
		[]

	ContinueStatement
		[]

	ReturnStatement
		[ Expression ]

	ExpressionStatement
		Expression

	Expression // the base expression type
		AssignExpression |
		TernaryExpression |
		BinaryExpression |
		UnaryExpression |
		ConstantExpression |
		SubExpression |

	SubExpression // an expression enclosed in parens; should be evaluated before the parent
		Expression

	AssignExpression
		Identifier, Operator, Expression

	TernaryExpression
		Expression, Expression, Expression

	BinaryExpression
		Expression, Operator, Expression

	UnaryExpression
		Expression, Operator
		
*/

map<AstNodeType, string> astNodeNames = {
	{ AST_SCRIPT, 			"Script" },
	{ AST_FUNC_DECL, 		"FuncDecl" },
	{ AST_VAR_STATEMENT,	"VarStatement" },
	{ AST_IMPORT_STATEMENT, "ImportStatement" },

	{ AST_IDENTIFIER,		"Identifier" },
	{ AST_CONST_STRING,		"StringConstant" },
	{ AST_CONST_NUMBER, 	"NumberConstant" },
	{ AST_EXPRESSION,		"Expression" },
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
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_VAR_STATEMENT);

	if (symbol->type == SYMBOL_VAR)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_NAME)
	{
		AstNode* name = new AstNode(AST_IDENTIFIER, symbol->data);
		node->children.push_back(name);
		symbol++;
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_SEMICOLON)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	return node;

fail:
	delete node;
	symbol = orig;
	return nullptr;
}

AstNode* parseExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_EXPRESSION);

	if (symbol->type == SYMBOL_NUMBER)
	{
		AstNode* num = new AstNode(AST_CONST_NUMBER, symbol->data);
		node->children.push_back(num);
		symbol++;
	}
	else if (symbol->type == SYMBOL_STRING)
	{
		AstNode* str = new AstNode(AST_CONST_STRING, symbol->data);
		node->children.push_back(str);
		symbol++;
	}
	else
	{
		goto fail;
	}

	return node;

fail:
	delete node;
	symbol = orig;
	return nullptr;	
}

AstNode* parseVarStatement2(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_VAR_STATEMENT);
	AstNode* child = nullptr;

	if (symbol->type == SYMBOL_VAR)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_NAME)
	{
		AstNode* name = new AstNode(AST_IDENTIFIER, symbol->data);
		node->children.push_back(name);
		symbol++;
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_OP_ASSIGN)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	child = parseExpression(symbol);
	if (child)
	{
		node->children.push_back(child);
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_SEMICOLON)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	return node;

fail:
	delete node;
	symbol = orig;
	return nullptr;
}

AstNode* parseImportStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_IMPORT_STATEMENT);

	if (symbol->type == SYMBOL_IMPORT)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_NAME)
	{
		AstNode* name = new AstNode(AST_IDENTIFIER, symbol->data);
		node->children.push_back(name);
		symbol++;
	}
	else
	{
		goto fail;
	}

	if (symbol->type == SYMBOL_SEMICOLON)
	{
		symbol++;
	}
	else
	{
		goto fail;
	}

	return node;

fail:
	delete node;
	symbol = orig;
	return nullptr;
}



AstNode* parseScript(vector<Symbol>& symbols)
{
	SymbolIter symbol = symbols.begin();

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

		child = parseVarStatement2(symbol);
		if (child)
		{
			node->children.push_back(child);
			continue;
		}

		// child = parseFuncDecl(symbol);
		// if (child)
		// {
		// 	node->children.push_back(child);
		// 	continue;
		// }

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
		printAst(node->children[i], indent+"    ");
	}
}

#endif