#include "sno_state.h"

#include <stdlib.h>
#include <string.h>

sno_API sno_State* sno_create_state(void) {
	sno_State* state = malloc(sizeof(sno_State));
	if (!state) {
		return NULL;
	}
	state->mem_allocated = 0;
	state->num_allocations = 0;
	return state;
}

sno_API void sno_free_state(sno_State* state) {
	sno_assert_ptr(state);
	free(state);
}

sno_API sno_no_return void sno_throw_runtime_error(sno_VM* vm) {
	sno_assert_ptr(vm);
}

sno_no_return void sno_throw(
	sno_VM* vm,
	ExceptionType type,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(message);
	sno_assert_ptr(type != EXCEPTION_NONE);
	sno_assert_ptr(type < NUM_EXCEPTION_TYPES);
	if (message_length > EXCEPTION_MESSAGE_MAX_LENGTH) {
		message_length = EXCEPTION_MESSAGE_MAX_LENGTH;
	}
	vm->exception_type = type;
	vm->exception_message_length = message_length;
	memcpy(vm->exception_message, message, message_length);
	longjmp(vm->exception_jump->buf, type);
}
