#ifndef _TACK_PARSE_H
#define _TACK_PARSE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"

typedef enum AstNodeType {
    AST_UNKNOWN = 0,
    
    AST_SCRIPT,
    AST_IMPORT,
    AST_FUNC_DECL,
    AST_ARGS_DECL,
    AST_IFCONSTRUCT,
    AST_IFBLOCK,
    AST_ELSEIFBLOCK,
    AST_ELSEBLOCK,
    AST_WHILE,
    AST_STATEMENTBLOCK,
    AST_STATEMENT,
    AST_VAR_STATEMENT,
    AST_BREAK_STATEMENT,

    AST_EXP_CONDITIONAL,
    AST_IDENTIFIER,
    AST_NUMBER_LITERAL,

} AstNodeType;

typedef struct CompileError {
    int line;
    char* msg;
} CompileError;

typedef struct ParseContext {
    const char* code;
    int length;
    int pos;
    CompileError* errors;
} ParseContext;

typedef struct AstNode {
    struct AstNode** children;
    int num_children;
    AstNodeType type;
    char* data;
} AstNode;


AstNode* ast_new(AstNodeType type) {
    AstNode* n = NEW(AstNode);
    n->type = type;
    n->data = NULL;
    n->children = NULL;
    n->num_children = 0;
    return n;
}
void ast_free(AstNode* node) {
    if (node) {
        if (node->children) {
            for (int i = 0; i < node->num_children; i++) {
                ast_free(node->children[i]);
            }
        }
        FREE(node->children);
        FREE(node->data);
    }
    FREE(node);
}
void ast_print(AstNode* node, int level) {
    if (node) {
        switch (node->type) {
            case AST_IDENTIFIER:
                printf("Identifier: %s\n", node->data);
                break;
            case AST_NUMBER_LITERAL:
                printf("Number literal: %s\n", node->data);
                break;
            case AST_EXP_CONDITIONAL:
                printf("conditional:\n");
                break;
            default:
                printf("Unknown node type\n");
                break;
        }
    }
}



bool is_identifier_starter(char c) {
    return  (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c == '_');
}
bool is_identifier_char(char c) {
    return  is_identifier_starter(c) ||
            (c >= '0' && c <= '9');
}

void skip_whitespace(ParseContext* ctx) {
    while (ctx->pos < ctx->length) {
        char c = ctx->code[ctx->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v') {
            ctx->pos++;
        } else {
            break;
        }
    }
}

// parsing functions
AstNode* parse_identifier(ParseContext* ctx) {

    int startpos = ctx->pos;
    char c = ctx->code[ctx->pos];
    
    if (is_identifier_starter(c)) {
        ctx->pos += 1;
        while (is_identifier_char(ctx->code[ctx->pos])) {
            ctx->pos += 1;
        }

        AstNode* node = ast_new(AST_IDENTIFIER);
        int datasize = ctx->pos - startpos;
        
        node->data = NEW_ARRAY(char, datasize + 1);
        node->data[datasize] = 0;
        memcpy(node->data, &ctx->code[startpos], datasize);

        return node;
    }

    ctx->pos = startpos;
    return NULL;
}
AstNode* parse_int_literal(ParseContext* ctx) {

    // save start position
    int startpos = ctx->pos;

    // advance over all digits in a row
    while (ctx->code[ctx->pos] <= '9' && ctx->code[ctx->pos] >= '0') {
        ctx->pos += 1;
    }

    // if there were some digits, this is an integer literal
    if (ctx->pos > startpos) {
        AstNode* node = ast_new(AST_NUMBER_LITERAL);
        int datasize = ctx->pos - startpos;
        node->data = NEW_ARRAY(char, datasize + 1);
        node->data[datasize] = 0;
        memcpy(node->data, &ctx->code[startpos], datasize);
        return node;
    } else {
        ctx->pos = startpos;
        return NULL;
    }
}
AstNode* parse_primary_exp(ParseContext* ctx) {
    AstNode* node = parse_identifier(ctx);
    if (!node) {
        return parse_int_literal(ctx);
    }
    return node;
}
AstNode* parse_bool_exp(ParseContext* ctx) {
    AstNode* node = parse_primary_exp(ctx);
    if (node) {

    }
    return nullptr;
}

AstNode* parse_ternary_exp(ParseContext* ctx) {
    skip_whitespace(ctx);

    int start = ctx->pos;

    AstNode* cond = NULL;
    AstNode* exp1 = NULL;
    AstNode* exp2 = NULL;

    // start with the conditional expression
    cond = parse_bool_exp(ctx);
    if (cond) {
        skip_whitespace(ctx);

        // if a question mark is found, this must be a ternary expression
        if (ctx->code[ctx->pos] == '?') {
            ctx->pos += 1;
            skip_whitespace(ctx);

            // we expect to find an expression
            AstNode* exp1 = parse_bool_exp(ctx);
            if (exp1) {
                skip_whitespace(ctx);
                if (ctx->code[ctx->pos] == ':') {
                    ctx->pos += 1;
                    skip_whitespace(ctx);
                    AstNode* exp2 = parse_bool_exp(ctx);
                    if (exp2) {
                        AstNode* ast = ast_new(AST_EXP_CONDITIONAL);
                        ast->children = NEW_ARRAY(AstNode*, 3);
                        ast->children[0] = cond;
                        ast->children[1] = exp1;
                        ast->children[2] = exp2;
                        ast->data = NULL;
                        return ast;
                    } else {
                        // TODO: error, expected to find exp2
                        printf("ERROR: ternary expression requires 2 terms\n");
                    }
                } else {
                    // TODO: error, there was no colon
                    printf("ERROR: ternary expression expected ':'\n");
                }
            } else {
                // TODO: error, expected to find exp1
                printf("ERROR: ternary expression expected 2 terms\n");
            }
            ast_free(exp1);
            ast_free(exp2);
        }
        // question mark not found; this is a normal boolean expression
        return cond;
    }
    
    ctx->pos = start;
    return NULL;
}

AstNode* parse_expression(ParseContext* ctx) {
    return parse_ternary_exp(ctx);
}

AstNode* parse(const char* code) {
    ParseContext ctx = {
        .code = code,
        .length = strlen(code),
        .pos = 0,
    };

    AstNode* ast = parse_expression(&ctx);
    return ast;
}

#endif