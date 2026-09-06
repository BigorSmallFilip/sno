#include "sno_state.h"

#include "sno_mem.h"

sno_API sno_State* sno_create_state() {
	sno_State* state = sno_malloc(NULL, sizeof(sno_State));
	sno_init_string_interning_table(state, 64);
	return state;
}

sno_API void sno_free_state(sno_State* state) {
	if (!state) {
		return;
	}
}

sno_API sno_no_return void sno_throw(sno_State* state, const sno_String* exception_msg) {
	sno_assert_ptr(state);
	sno_assert_ptr(exception_msg);
	if (state->exception_jump) {
		state->exception_msg = exception_msg;
		longjmp(state->exception_jump->buf, 1);
	} else {
		fputs(sno_ANSI_RED "Uncaught exception thrown!\n" sno_ANSI_NORMAL, stderr);
		exit(EXIT_FAILURE);
	}
}
