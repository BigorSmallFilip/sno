#include "sno_state.h"

#include <stdlib.h>
#include <string.h>

sno_API sno_GlobalState* sno_create_state(void) {
	sno_GlobalState* state = malloc(sizeof(sno_GlobalState));
	if (!state) {
		return NULL;
	}
	state->mem_allocated = 0;
	state->num_allocations = 0;
	init_string_interning_table(state, 64);
	return state;
}

sno_API void sno_free_state(sno_GlobalState* state) {
	sno_assert_ptr(state);
	free(state);
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

sno_API sno_no_return void sno_throw_runtime_error(
	sno_VM* vm,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(message);
	sno_throw(vm, EXCEPTION_RUNTIME_ERROR, message, message_length);
}



sno_API void sno_run_test_thing(sno_GlobalState* state) {
	sno_assert_ptr(state);

	(void)create_string(state, sno_string_comma_length("What"));
	(void)create_string(state, sno_string_comma_length("Is"));
	(void)create_string(state, sno_string_comma_length("Even"));
	(void)create_string(state, sno_string_comma_length("Going"));
	(void)create_string(state, sno_string_comma_length("On?"));
	(void)create_string(state, sno_string_comma_length("xyz"));

	print_string_interning_table(state);
}
