#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "allocator.h"
#include "parse.h"

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
    
    char* data = NEW_ARRAY(char, size+1);
    data[size] = 0;
    fread(data, size, sizeof(char), file);
    fclose(file);
    
    return data;
}


typedef struct State {
    int unused;
} State;



State* state_new() {
    return NULL;
}
void state_free(State* state)  {
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
    
    if (argc == 2) {
        
        printf("Executing %s\n", argv[1]);
        char* code = read_text_file(argv[1]);
        
        State* state = state_new();
        execute(state, code);
        state_free(state);
        
        FREE(code);
        
    } else if (argc == 1) {
        printf("entering repl mode\n");

        // todo: unsafe? defo unscalable.
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