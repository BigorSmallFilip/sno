#include "sno_mem.h"

#include "sno_state.h"
#include <stdlib.h>

void* state_alloc(sno_State* state, size_t size) {
	sno_assert_ptr(state);
	state->mem_allocated += size;
	state->num_allocations++;
	void* block = malloc(size);
	if (!block) {
		fputs("Allocation failed", stderr);
	}
	return block;
}

void state_free(sno_State* state, size_t size, void* block) {
	sno_assert_ptr(state);
	sno_assert_ptr(block);
	sno_assert(state->num_allocations > 0);
	sno_assert(state->mem_allocated > 0);
	state->mem_allocated -= size;
	state->num_allocations--;
	free(block);
}
