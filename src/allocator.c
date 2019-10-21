#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "allocator.h"



// typedef struct AllocNode {
//     void* ptr;
//     char* file;
//     int line;
// } AllocNode;

// typedef struct AllocTable {
//     AllocNode* nodes;
//     int count;
//     int capacity;
// } AllocTable;

// AllocTable table = {
//     .nodes = NULL,
//     .capacity = 0,
//     .count = 0
// };


// void debug_alloc_begin() {
//     table.nodes = malloc(sizeof(AllocNode));
//     table.capacity = 1;
// }

// void debug_alloc_end() {
//     for (int i = 0; i < table.count; i++) {
//         printf("ERROR: Unfreed allocation at %s (%d)\n", table.nodes[i].file, table.nodes[i].line);
//     }

//     free(table.nodes);
// }

// void* debug_malloc(int size, const char* file, int line) {
//     // track a new allocation
//     AllocNode* node = malloc(sizeof(AllocNode));
//     node->ptr = malloc(size);
//     node->file = file;
//     node->line = line;

//     // add to list
//     //alloc_tail.prev->next = node;
//     //alloc_tail.prev = node;
    
//     return node->ptr;
// }
// void* debug_realloc(void* ptr, int size) {
//     if (!ptr) {
//         printf("ERROR: trying to realloc null pointer!\n");
//         return;
//     }
//     // find the existing pointer
//     // record an error if it didn't exist (but still do the requested reallocation)
//     AllocNode* node = debug_alloc_findptr(ptr);
//     if (!node) {
//         printf("ERROR: debug allocator trying to resize untracked pointer %p", ptr);
//         return realloc(ptr, size);
//     }

//     // reallocate to our tracked alloc
//     node->ptr = realloc(node->ptr, size);
//     return node->ptr;
// }
// void debug_free(void* ptr) {
//     if (!ptr) {
//         printf("ERROR: trying to free null pointer!\n");
//     }
//     // find the existing pointer
//     // record an error if not found (but still do the free)
//     AllocNode* node = debug_alloc_findptr(ptr);
//     if (!node) {
//         printf("ERROR: debug allocator trying to free untracked pointer %p", ptr);
//         free(ptr);
//         return;
//     }

//     // remove node from list
//     // if (node->prev) {
//     //     node->prev->next = node->next;
//     // }
//     // if (node->next) {
//     //     node->next->prev = node->prev;
//     // }

//     free(node->ptr);
//     free(node);
// }
