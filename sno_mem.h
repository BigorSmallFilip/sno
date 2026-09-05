#ifndef sno_MEM_H
#define sno_MEM_H

#include "sno_utility.h"

void* sno_malloc(struct sno_State* state, size_t size);
void* sno_calloc(struct sno_State* state, size_t count, size_t size);
void* sno_realloc(struct sno_State* state, void* block, size_t new_size);
#define sno_alloc_type(state, type) (type*)sno_calloc(state, 1, sizeof(type))
void sno_free(struct sno_State* state, void* block);



typedef struct {
	void* buffer;
	size_t count;
	size_t capacity;
} sno_DynArray;

#define sno_MIN_DYNARRAY_CAPACITY 2

void sno_dynarray_init(struct sno_State* state, sno_DynArray* dynarray, size_t element_size, size_t capacity);
void sno_dynarray_resize(struct sno_State* state, sno_DynArray* dynarray, size_t element_size, size_t new_capacity);
void sno_dynarray_push_back(struct sno_State* state, sno_DynArray* dynarray, size_t element_size, const void* sno_restrict data);
void sno_dynarray_push_back_no_resize(struct sno_State* state, sno_DynArray* dynarray, size_t element_size, const void* sno_restrict data);
void sno_dynarray_push_back_ptr(struct sno_State* state, sno_DynArray* dynarray, void* ptr);
void sno_dynarray_push_back_ptr_no_resize(struct sno_State* state, sno_DynArray* dynarray, void* ptr);
void sno_dynarray_pop_back(struct sno_State* state, sno_DynArray* dynarray, size_t element_size, void* sno_restrict data);
void* sno_dynarray_get(struct sno_State* state, sno_DynArray* dynarray, size_t element_size, size_t index);
void* sno_dynarray_get_ptr(struct sno_State* state, sno_DynArray* dynarray, size_t index);
void sno_dynarray_clear(struct sno_State* state, sno_DynArray* dynarray);

#endif
