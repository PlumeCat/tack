#ifndef _AST_H
#define _AST_H

/*
AST Node definitions:

	// { x } 	repeat x (zero or more)
	// x | y 	x or y
	// [ x ]	x is optional

	√ Script
		{ ImportStatement | FuncDecl | VarStatement }

	√ FuncDecl
		Identifier, ArgsDecl, StatementBlock

	√ ArgsDecl
		[ { Identifier } ]

	√ VarStatement
		Identifier, [ Expression ]

	√ ImportStatement
		Identifier

	√ IfConstruct
		IfBlock, [ ElseBlock ]

	√ IfBlock
		Expression, StatementBlock

	√ ElseBlock
		StatementBlock, IfConstruct


	√ WhileLoop
		Expression, StatementBlock

	√ StatementBlock
		{ 	
			√ IfConstruct | 
			√ WhileLoop | 
			√ BreakStatement | 
			√ ContinueStatement | 
			√ ReturnStatement | 
			VarStatement | 
			ExpressionStatement 
		}

	√ BreakStatement

	√ ContinueStatement

	√ ReturnStatement
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

	√ SubExpression // an expression enclosed in parens; should be evaluated before the parent
		Expression

	AssignExpression
		Identifier, Operator, Expression

	CompareExpression
		Identifier, Operator, Expression

	TernaryExpression
		Expression, Expression, Expression

	BinaryExpression
		Expression, Operator, Expression

	UnaryExpression
		Expression, Operator
		
*/

typedef vector<Symbol>::iterator SymbolIter;

enum AstNodeType
{
	AST_SCRIPT = 1,

	AST_FUNC_DECL,
	AST_VAR_STATEMENT,
	AST_IMPORT_STATEMENT,
	AST_ARGS_DECL,
	AST_STATEMENT_BLOCK,
	AST_IF_CONSTRUCT,
	AST_IF_BLOCK,
	AST_ELSE_BLOCK,
	AST_WHILE_CONSTRUCT,

	AST_RETURN_STATEMENT,
	AST_CONTINUE_STATEMENT,
	AST_BREAK_STATEMENT,

	AST_EXPRESSION,
	AST_SUB_EXPRESSION,
	AST_EXPRESSION_STATEMENT,
	AST_ASSIGN_EXPRESSION,
	AST_COMPARE_EXPRESSION,
	AST_BINARY_EXPRESSION,
	AST_TERNARY_EXPRESSION,
	AST_UNARY_EXPRESSION,

	AST_IDENTIFIER,
	AST_CONST_STRING,
	AST_CONST_NUMBER,

};


map<AstNodeType, string> astNodeNames = {
	{ AST_SCRIPT, 			"Script" },
	{ AST_FUNC_DECL, 		"FuncDecl" },
	{ AST_VAR_STATEMENT,	"VarStatement" },
	{ AST_IMPORT_STATEMENT, "ImportStatement" },

	{ AST_ARGS_DECL, 		"ArgsDecl" },
	{ AST_STATEMENT_BLOCK,	"StatementBlock" },
	{ AST_IF_CONSTRUCT ,	"IfConstruct"	},
	{ AST_IF_BLOCK,			"IfBlock" },
	{ AST_ELSE_BLOCK,		"ElseBlock" },
	{ AST_WHILE_CONSTRUCT,	"WhileConstruct" },

	{ AST_RETURN_STATEMENT,	"ReturnStatement" },
	{ AST_CONTINUE_STATEMENT,"ContinueStatement" },
	{ AST_BREAK_STATEMENT,	"BreakStatement" },

	{ AST_IDENTIFIER,		"Identifier" },
	{ AST_EXPRESSION,		"Expression" },
	{ AST_SUB_EXPRESSION,	"SubExpression" },
	{ AST_EXPRESSION_STATEMENT, "ExpressionStatement" },

	{ AST_ASSIGN_EXPRESSION,"AssignExpression" },
	{ AST_COMPARE_EXPRESSION,"CompareExpression" },
	{ AST_BINARY_EXPRESSION,"BinaryExpression" },
	{ AST_UNARY_EXPRESSION, "UnaryExpression" },
	{ AST_TERNARY_EXPRESSION,"TernaryExpression" },

	{ AST_CONST_STRING,		"StringConstant" },
	{ AST_CONST_NUMBER, 	"NumberConstant" },

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


typedef AstNode* (*AstNodeFunc)(SymbolIter& iter);
bool expect(SymbolIter& iter, AstNodeFunc func, AstNode* parent)
{
	SymbolIter orig = iter;
	AstNode* child = func(iter);
	if (child)
	{
		parent->children.push_back(child);
		return true;
	}

	iter = orig;
	return false;
}
bool expect(SymbolIter& iter, SymbolType type)
{
	if (iter->type == type)
	{
		iter++;
		return true;
	}
	return false;
}
bool expect(SymbolIter& iter, SymbolType type, string& outData)
{
	if (iter->type == type)
	{
		outData = iter->data;
		iter++;
		return true;
	}

	return false;
}

AstNode* parseImportStatement(SymbolIter&);
AstNode* parseFuncDecl(SymbolIter&);
AstNode* parseArgsDecl(SymbolIter&);
AstNode* parseVarStatement(SymbolIter&);
AstNode* parseIfConstruct(SymbolIter&);
AstNode* parseIfBlock(SymbolIter&);
AstNode* parseElseBlock(SymbolIter&);
AstNode* parseWhileLoop(SymbolIter&);
AstNode* parseStatementBlock(SymbolIter&);
AstNode* parseBreakStatement(SymbolIter&);
AstNode* parseContinueStatement(SymbolIter&);
AstNode* parseReturnStatement(SymbolIter&);
AstNode* parseExpressionStatement(SymbolIter&);
AstNode* parseExpression(SymbolIter&);
AstNode* parseSubExpression(SymbolIter&);
AstNode* parseAssignExpression(SymbolIter&);
AstNode* parseTernaryExpression(SymbolIter&);
AstNode* parseBinaryExpression(SymbolIter&);
AstNode* parseUnaryExpression(SymbolIter&);
AstNode* parseCompareExpression(SymbolIter&);


AstNode* parseExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_EXPRESSION);

	if (symbol->type == SYMBOL_NUMBER)
	{
		AstNode* num = new AstNode(AST_CONST_NUMBER, symbol->data);
		node->children.push_back(num);
		symbol++;
		return node;
	}
	else if (symbol->type == SYMBOL_STRING)
	{
		AstNode* str = new AstNode(AST_CONST_STRING, symbol->data);
		node->children.push_back(str);
		symbol++;
		return node;
	}
	else if (symbol->type == SYMBOL_LPAREN)
	{
		AstNode* sub = parseSubExpression(symbol);
		node->children.push_back(sub);
		return node;
	}

	delete node;
	symbol = orig;
	return nullptr;	
}

AstNode* parseExpressionStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_EXPRESSION_STATEMENT);

	if (expect(iter, parseExpression, node))
	if (expect(iter, SYMBOL_SEMICOLON))
		return node;

	delete node;
	symbol = orig;
	return nullptr;	
}
AstNode* parseAssignExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;

}
AstNode* parseCompareExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_COMPARE_EXPRESSION);

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseBinaryExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_BINARY_EXPRESSION);

	if (expect(symbol, parseExpression, node))
	if (expect(symbol, SYMBOL_OP_BOOLOR) ||
		expect(symbol, SYMBOL_OP_BOOLAND) ||
		expect(symbol, SYMBOL_OP_BITOR) ||
		expect(symbol, SYMBOL_OP_BITAND) ||
		expect(symbol, SYMBOL_OP_BITXOR) ||
		expect(symbol, SYMBOL_OP_LSHIFT) ||
		expect(symbol, SYMBOL_OP_RSHIFT) ||
		expect(symbol, SYMBOL_OP_ADD) ||
		expect(symbol, SYMBOL_OP_SUB) ||
		expect(symbol, SYMBOL_OP_MUL) ||
		expect(symbol, SYMBOL_OP_DIV) ||
		expect(symbol, SYMBOL_OP_MOD))
	if (expect(symbol, parseExpression, node))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseUnaryExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_BINARY_EXPRESSION);

	if (expect(symbol, parseExpression, node))
	if (expect(symbol, SYMBOL_OP_INCREMENT) ||
		expect(symbol, SYMBOL_OP_DECREMENT) ||
		expect(symbol, SYMBOL_OP_BOOLNOT) ||
		expect(symbol, SYMBOL_OP_BITNOT))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseTernaryExpression(SymbolIter& symbol)
{

}
AstNode* parseIdentifier(SymbolIter& iter)
{
	string data;
	if (expect(iter, SYMBOL_NAME, data))
	{
		AstNode* node = new AstNode(AST_IDENTIFIER, data);
		return node;
	}
	return nullptr;
}

AstNode* parseImportStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_IMPORT_STATEMENT);

	if (expect(symbol, SYMBOL_IMPORT))
	if (expect(symbol, parseIdentifier, node))
	if (expect(symbol, SYMBOL_SEMICOLON))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}


AstNode* parseVarStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_VAR_STATEMENT);

	if (expect(symbol, SYMBOL_VAR))
	if (expect(symbol, parseIdentifier, node))
	{
		if (expect(symbol, SYMBOL_SEMICOLON))
			return node;
		else 
		{
			if (expect(symbol, SYMBOL_OP_ASSIGN))
			if (expect(symbol, parseExpression, node))
			if (expect(symbol, SYMBOL_SEMICOLON))
				return node;
		}
	}

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseArgsDecl(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_ARGS_DECL);

	if (expect(symbol, SYMBOL_LPAREN))
	while (expect(symbol, parseIdentifier, node) && 
		   expect(symbol, SYMBOL_COMMA)) {}
	if (expect(symbol, SYMBOL_RPAREN))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}

AstNode* parseIfBlock(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_IF_BLOCK);

	if (expect(symbol, SYMBOL_IF))
	if (expect(symbol, SYMBOL_LPAREN))
	if (expect(symbol, parseExpression, node))
	if (expect(symbol, SYMBOL_RPAREN))
	if (expect(symbol, parseStatementBlock, node))
		return node;

	symbol = orig;
	delete node;
	return nullptr;
}
AstNode* parseElseBlock(SymbolIter& symbol)
{
	SymbolIter& orig = symbol;
	AstNode* node = new AstNode(AST_ELSE_BLOCK);

	if (expect(symbol, SYMBOL_ELSE))
	if (expect(symbol, parseStatementBlock, node) ||
		expect(symbol, parseIfConstruct, node))
		return node;

	symbol = orig;
	delete node;
	return nullptr;
}
AstNode* parseIfConstruct(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_IF_CONSTRUCT);

	if (expect(symbol, parseIfBlock, node))
	if (expect(symbol, parseElseBlock, node) || true)
		return node;

	symbol = orig;
	delete node;
	return nullptr;
}
AstNode* parseWhileLoop(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_WHILE_CONSTRUCT);

	if (expect(symbol, SYMBOL_WHILE))
	if (expect(symbol, SYMBOL_LPAREN))
	if (expect(symbol, parseExpression, node))
	if (expect(symbol, SYMBOL_RPAREN))
	if (expect(symbol, parseStatementBlock, node))
		return node;

	symbol = orig;
	delete node;
	return nullptr;
}
AstNode* parseSubExpression(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_SUB_EXPRESSION);

	if (expect(symbol, SYMBOL_LPAREN))
	if (expect(symbol, parseExpression, node))
	if (expect(symbol, SYMBOL_RPAREN))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseStatementBlock(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_STATEMENT_BLOCK);

	if (expect(symbol, SYMBOL_LBRACE))
	{
		while (expect(symbol, parseVarStatement, node) ||
			   expect(symbol, parseIfConstruct, node) ||
			   expect(symbol, parseWhileLoop, node) || 
			   expect(symbol, parseReturnStatement, node) ||
			   expect(symbol, parseBreakStatement, node) ||
			   expect(symbol, parseContinueStatement, node)) {}
		if (expect(symbol, SYMBOL_RBRACE))
			return node;
	}

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseReturnStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_RETURN_STATEMENT);

	if (expect(symbol, SYMBOL_RETURN))
	{
		if (expect(symbol, parseExpression, node))
		{
			if (expect(symbol, SYMBOL_SEMICOLON))
			{
				return node;
			}
		}
		else
		{
			if (expect(symbol, SYMBOL_SEMICOLON))
			{
				return node;
			}
		}
	}


	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseBreakStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_BREAK_STATEMENT);

	if (expect(symbol, SYMBOL_BREAK))
	if (expect(symbol, SYMBOL_SEMICOLON))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseContinueStatement(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_CONTINUE_STATEMENT);

	if (expect(symbol, SYMBOL_CONTINUE))
	if (expect(symbol, SYMBOL_SEMICOLON))
		return node;

	delete node;
	symbol = orig;
	return nullptr;
}
AstNode* parseFuncDecl(SymbolIter& symbol)
{
	SymbolIter orig = symbol;
	AstNode* node = new AstNode(AST_FUNC_DECL);

	if (expect(symbol, SYMBOL_FUNCTION))
	if (expect(symbol, parseIdentifier, node))
	if (expect(symbol, parseArgsDecl, node))
	if (expect(symbol, parseStatementBlock, node))
		return node;

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
		if (expect(symbol, parseImportStatement, node) ||
			expect(symbol, parseVarStatement, node) ||
			expect(symbol, parseFuncDecl, node))
		{
			continue;	
		}
		else
		{
			delete node;
			return nullptr;
		}
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