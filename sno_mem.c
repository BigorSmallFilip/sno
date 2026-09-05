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
