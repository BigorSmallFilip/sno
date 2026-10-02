#include "sno_mem.h"

#include "sno_state.h"
#include <stdlib.h>
#include <stdio.h>

void* state_alloc(sno_GlobalState* state, size_t size) {
	sno_assert_ptr(state);
	state->mem_allocated += size;
	state->num_allocations++;
	void* block = malloc(size);
	if (!block) {
		fputs("Allocation failed", stderr);
	}
	return block;
}

void* state_realloc(
	sno_GlobalState* state,
	size_t old_size,
	void* block,
	size_t new_size
) {
	sno_assert_ptr(state);
	state->mem_allocated -= old_size;
	state->mem_allocated += new_size;
	void* new_block = realloc(block, new_size);
	if (!new_block) {
		fputs(sno_ANSI_RED "Reallocation failed\n" sno_ANSI_NORMAL, stderr);
	}
	return new_block;
}

void state_free(sno_GlobalState* state, size_t size, void* block) {
	sno_assert_ptr(state);
	sno_assert_ptr(block);
	sno_assert(state->num_allocations > 0);
	sno_assert(state->mem_allocated > 0);
	state->mem_allocated -= size;
	state->num_allocations--;
	free(block);
}



DEFINE_GENERIC_DYN_ARRAY(uint8_t, Byte, byte);
DEFINE_GENERIC_DYN_ARRAY(sno_Number, Number, number);
