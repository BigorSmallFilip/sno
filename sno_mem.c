#include "sno_mem.h"

#include "sno_state.h"
#include <stdlib.h>
#include <string.h>

void* sno_malloc(sno_State* state, size_t size) {
	void* block = malloc(size);
	if (!block) {
		sno_panic("Allocation failed");
	}
	return block;
}

void* sno_calloc(sno_State* state, size_t count, size_t size) {
	void* block = calloc(count, size);
	if (!block) {
		sno_panic("Allocation failed");
	}
	return block;
}

void* sno_realloc(sno_State* state, void* block, size_t new_size) {
	sno_assert_ptr(block);
	void* new_block = realloc(block, new_size);
	if (!new_block) {
		sno_panic("Allocation failed");
	}
	return new_block;
}

void sno_free(sno_State* state, void* block) {
	sno_assert_ptr(block);
	free(block);
}



void sno_dynarray_init(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	size_t capacity
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert(capacity > 1);
	sno_assert(sno_is_power_of_2(capacity));
	dynarray->buffer = sno_calloc(state, capacity, element_size);
	sno_assert_ptr(dynarray->buffer);
	dynarray->capacity = capacity;
	dynarray->count = 0;
}

void sno_dynarray_resize(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	size_t new_capacity
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert(new_capacity >= dynarray->count);
	sno_assert(new_capacity > 1);
	sno_assert(sno_is_power_of_2(new_capacity));
	if (dynarray->capacity == new_capacity) return;
	dynarray->buffer = sno_realloc(state, dynarray->buffer, new_capacity * element_size);
	dynarray->capacity = new_capacity;
}

void sno_dynarray_push_back(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	const void* sno_restrict data
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert_ptr(data);
	sno_assert(dynarray->count <= dynarray->capacity);
	if (dynarray->count == dynarray->capacity) {
		sno_dynarray_resize(state, dynarray, element_size, dynarray->capacity << 1);
	}
	(void)memcpy((void*)((uintptr_t)dynarray->buffer + dynarray->count * element_size), data, element_size);
	dynarray->count++;
}

void sno_dynarray_push_back_no_resize(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	const void* sno_restrict data
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert_ptr(data);
	sno_assert(dynarray->count + 1 <= dynarray->capacity);
	(void)memcpy((void*)((uintptr_t)dynarray->buffer + dynarray->count * element_size), data, element_size);
	dynarray->count++;
}

void sno_dynarray_push_back_ptr(
	sno_State* state,
	sno_DynArray* dynarray,
	const void* sno_restrict ptr
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert(dynarray->count <= dynarray->capacity);
	if (dynarray->count == dynarray->capacity) {
		sno_dynarray_resize(state, dynarray, sizeof(void*), dynarray->capacity << 1);
	}
	((const void**)dynarray->buffer)[dynarray->count] = ptr;
	dynarray->count++;
}

void sno_dynarray_push_back_ptr_no_resize(
	sno_State* state,
	sno_DynArray* dynarray,
	const void* sno_restrict ptr
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert(dynarray->count + 1 <= dynarray->capacity);
	((const void**)dynarray->buffer)[dynarray->count] = ptr;
	dynarray->count++;
}

void sno_dynarray_pop_back(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	void* sno_restrict data
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert_ptr(data);
	sno_assert(dynarray->count > 0);
	dynarray->count--;
	(void)memcpy(data, (uint8_t*)dynarray->buffer + dynarray->count * element_size, element_size);
	if (dynarray->capacity > 4 && dynarray->count < (dynarray->capacity >> 2)) {
		sno_dynarray_resize(state, dynarray, element_size, dynarray->capacity >> 1);
	}
}

void* sno_dynarray_get(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	size_t index
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert(index < dynarray->count);
	return (void*)((uintptr_t)dynarray->buffer + index * element_size);
}

void* sno_dynarray_get_ptr(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t index
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_assert(index < dynarray->count);
	return ((void**)dynarray->buffer)[index];
}

void sno_dynarray_clear(
	sno_State* state,
	sno_DynArray* dynarray
) {
	sno_assert_ptr(state);
	sno_assert_ptr(dynarray);
	sno_assert_ptr(dynarray->buffer);
	sno_assert(dynarray->capacity >= sno_MIN_DYNARRAY_CAPACITY);
	sno_free(state, dynarray->buffer);
}
