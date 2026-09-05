#ifndef sno_MEM_H
#define sno_MEM_H

#include "sno_utility.h"

void* sno_malloc(struct sno_State* state, size_t size);
void* sno_calloc(struct sno_State* state, size_t count, size_t size);
void* sno_realloc(struct sno_State* state, void* block, size_t new_size);
#define sno_alloc_type(state, type) (type*)sno_calloc(state, 1, sizeof(type))
void sno_free(struct sno_State* state, void* block);



#endif
