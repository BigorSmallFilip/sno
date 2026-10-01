#ifndef sno_MEM_H
#define sno_MEM_H

#include "sno.h"

void* state_alloc(sno_State* state, size_t size);
void state_free(sno_State* state, size_t size, void* block);

#endif
