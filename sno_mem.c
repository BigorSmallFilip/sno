#include "sno_mem.h"

#include "sno_state.h"
#include <stdlib.h>

void* sno_malloc(sno_State* state, size_t size) {
	sno_assert_ptr(state);
	void* block = malloc(size);
	if (!block) {
		sno_panic("Allocation failed");
	}
	return block;
}

void* sno_calloc(sno_State* state, size_t count, size_t size) {
	sno_assert_ptr(state);
	void* block = calloc(count, size);
	if (!block) {
		sno_panic("Allocation failed");
	}
	return block;
}

void* sno_realloc(sno_State* state, void* block, size_t new_size) {
	sno_assert_ptr(state);
	sno_assert_ptr(block);
	void* new_block = realloc(block, new_size);
	if (!new_block) {
		sno_panic("Allocation failed");
	}
	return new_block;
}

void sno_free(sno_State* state, void* block) {
	sno_assert_ptr(state);
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
	Sno_AssertPtr(dynarray);
	Sno_Assert(newcapacity >= dynarray->count);
	if (dynarray->capacity == newcapacity) return;
	dynarray->buffer = Sno_Realloc(dynarray->buffer, newcapacity * elementsize);
	dynarray->capacity = newcapacity;
}

void sno_dynarray_push_back(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	const void* sno_restrict data
) {

}

void sno_dynarray_push_back_no_resize(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	const void* sno_restrict data
) {

}

void sno_dynarray_push_back_ptr(
	sno_State* state,
	sno_DynArray* dynarray,
	void* ptr
) {

}

void sno_dynarray_push_back_ptr_no_resize(
	sno_State* state,
	sno_DynArray* dynarray,
	void* ptr
) {

}

void sno_dynarray_pop_back(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	const void* sno_restrict data
) {

}

void* sno_dynarray_get(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t element_size,
	size_t index
) {
	return nullptr;
}

void* sno_dynarray_get_ptr(
	sno_State* state,
	sno_DynArray* dynarray,
	size_t index
) {
	return nullptr;
}

void sno_dynarray_clear(
	sno_State* state,
	sno_DynArray* dynarray
) {

}
