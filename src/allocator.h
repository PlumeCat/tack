#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>



#if 0

// debug wrappers for allocation
// we keep track of the file

#warning "DEBUG MODE"

void debug_alloc_begin();
void* debug_alloc(int size, const char* file, int line);
void* debug_realloc(void* ptr, int newsize);
void  debug_free(void* ptr);
void debug_alloc_end();

#define NEW(Type) \
    (Type*)debug_alloc(sizeof(Type), __FILE__, __LINE__)

#define NEW_ARRAY(Type, Count) \
    (Type*)debug_alloc(sizeof(Type), __FILE__, __LINE__)

#define RESIZE_ARRAY(Ptr, Type, NewCount) \
    (Type*)debug_realloc(Ptr, sizeof(Type) * Count)

#define FREE(Ptr) \
    debug_free(Ptr)



#else

#define NEW(Type) (Type*)malloc(sizeof(Type))
#define NEW_ARRAY(Type, Count) (Type*)malloc(sizeof(Type) * Count)
#define RESIZE_ARRAY(Ptr, Type, NewCount) (Type*)realloc(Ptr, sizeof(Type) * Count)
#define FREE(Ptr) free(Ptr)

#endif // #ifdef _DEBUG
#endif // header guard