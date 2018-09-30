#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define new(Type) ((Type*)malloc(sizeof(Type)))
#define new_array(Type, Count) ((Type*)malloc(sizeof(Type) * Count))
#define resize_array(Arr, Type, Count) ((Type*)realloc(Arr, sizeof(Type) * Count))
#define true 1
#define false 0
#define bool int

/* replacement for gets_s(), sometimes unavailable??? */
int read_line(FILE* file, char* buf, int max) {
    if (!buf) {
        return 0;
    }

    int pos = 0;
    while (pos < max) {
        int c = getc(file);
        if (c == EOF || c == '\n') {
            return pos;
        }
        if (c > CHAR_MAX || c < CHAR_MIN) {
            c = 254; // ascii black box for unknown
        }
        buf[pos] = c;
        pos += 1;
    }
    return pos;
}

/* read entire text file into memory, return the result */
char* read_text_file(const char* fname) {
    FILE* file = fopen(fname, "r");
    
    // TODO: error handling tbh fam
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* data = new_array(char, size+1);
    data[size] = 0;
    fread(data, size, sizeof(char), file);
    fclose(file);
    
    return data;
}

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

    AST_IDENTIFIER,
    AST_NUMBER_LITERAL,

} AstNodeType;

typedef struct ParseContext {
    const char* code;
    int length;
    int pos;
    //const char** errors;
    //int num_errors;
} ParseContext;

typedef struct AstNode {
    struct AstNode* children;
    int num_children;
    AstNodeType type;
    char* data;
} AstNode;

typedef struct StackFrame {
    struct StackFrame* next;
    void* dummy;
} StackFrame;
typedef struct CompileError {
    int line;
    char* msg;
} CompileError;




AstNode* ast_new(AstNodeType type) {
    AstNode* n = new(AstNode);
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
                ast_free(&node->children[i]);
            }
        }
        free(node->children);
        free(node->data);
    }
    free(node);
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
            default:
                printf("Unknown node type\n");
                break;
        }
    }
}





typedef struct State {
    StackFrame* stack; // stack base
} State;

State* state_new() {
    State* s = new(State);
    s->stack = NULL;
    return s;
}
void state_free(State* s) {
    if (s) {
        free(s);
    }
}




inline bool is_identifier_starter(char c) {
    return  (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c == '_');
}
inline bool is_identifier_char(char c) {
    return  is_identifier_starter(c) ||
            (c >= '0' && c <= '9');
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
        
        node->data = new_array(char, datasize + 1);
        node->data[datasize] = 0;
        memcpy(node->data, &ctx->code[startpos], datasize);

        return node;
    }

    return NULL;
}
AstNode* parse_int_literal(ParseContext* ctx) {
    int startpos = ctx->pos;
    char c = ctx->code[ctx->pos];

    if (c <= '9' && c >= '0') {
        ctx->pos += 1;
        while (ctx->code[ctx->pos] <= '9' && ctx->code[ctx->pos] >= '0') {
            ctx->pos += 1;
        }

        AstNode* node = ast_new(AST_NUMBER_LITERAL);
        int datasize = ctx->pos - startpos;
        node->data = new_array(char, datasize + 1);
        node->data[datasize] = 0;
        memcpy(node->data, &ctx->code[startpos], datasize);

        return node;
    }

    return NULL;
}
AstNode* parse_float_literal(ParseContext* ctx) {
    return NULL;
}

AstNode* parse(const char* code) {
    ParseContext ctx = {
        .code = code,
        .length = strlen(code),
        .pos = 0,
        //.errors = NULL,
        //.num_errors = NULL,
    };

    AstNode* ast = parse_identifier(&ctx);
    if (!ast) ast = parse_int_literal(&ctx);
    return ast;
}

void execute(State* state, char* code) {
    AstNode* ast = parse(code);
    if (ast) {
        ast_print(ast, 0);
    } else {
        printf("Failed to compile\n");
    }
    ast_free(ast);
}

int main(int argc, char** argv) {

    malloc(100);
    
    if (argc == 2) {
        
        printf("Executing %s\n", argv[1]);
        char* code = read_text_file(argv[1]);
        
        State* state = state_new();
        execute(state, code);
        state_free(state);
        
        free(code);
        
    } else if (argc == 1) {
        printf("entering repl mode\n");

        // todo: unsafe
        #define LINESIZE 999
        char line[LINESIZE+1] = { 0 };

        State* state = state_new();

        while (true) {
            memset(line, 0, LINESIZE);
            printf(">> ");
            int count = read_line(stdin, line, LINESIZE);
            if (strcmp(line, "exit") == 0) {
                printf("exiting...\n");
                break;
            } else if (count) {
                execute(state, line);
            }
        }

        state_free(state);

    } else {
        printf("Too many arguments, exiting...\n");
    }
    return 0;
}