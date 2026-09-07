#include "sno_state.h"

#include "sno_mem.h"
#include "sno_parser.h"
#include "sno_vm.h"
#include <stdarg.h>

static void init_stack(sno_State* state, uint32_t capacity);

sno_API sno_State* sno_create_state() {
	sno_State* state = sno_alloc_type(NULL, sno_State);
	sno_init_string_interning_table(state, 64);
	init_stack(state, 128);
	return state;
}

static void init_stack(sno_State* state, uint32_t capacity) {
	sno_assert_ptr(state);
	sno_assert_msg(capacity != 0, "Capcity must not be 0");
	sno_assert_msg(sno_is_power_of_2(capacity), "Capcity must be power of 2");
	sno_assert(capacity <= sno_MAX_STACK);

	state->stack = sno_malloc(state, capacity * sizeof(sno_Value));
	sno_assert_ptr(state->stack);
	state->stack_capacity = capacity;
	state->stack_base = 0;
	state->stack_top = 0;

	state->globals = sno_create_table(state, 64);
}

static void resize_stack(sno_State* state, uint32_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->stack);
	sno_assert(state->stack_capacity > 0);
	sno_assert_msg(new_capacity != 0, "Capcity must not be 0");
	sno_assert_msg(sno_is_power_of_2(new_capacity), "Capcity must be power of 2");
	sno_assert(new_capacity <= sno_MAX_STACK);

	state->stack = sno_realloc(state, state->stack, new_capacity * sizeof(sno_Value));
	sno_assert_ptr(state->stack);
	state->stack_capacity = new_capacity;
}

sno_API void sno_reserve_stack(sno_State* state, uint32_t slots) {
	state->stack_top += slots;
	if (state->stack_top > state->stack_capacity) {
		resize_stack(state, state->stack_capacity << 1);
	}
}

sno_API void sno_free_state(sno_State* state) {
	if (!state) {
		return;
	}
}



sno_API void sno_print_globals(const sno_State* state) {
	sno_assert_ptr(state);

}

sno_API void sno_print_exception_msg(const sno_State* state) {
	sno_assert_ptr(state);
	fprintf(
		stderr,
		"%.*s\n",
		(unsigned int)state->exception_msg->length,
		sno_string_chars(state->exception_msg)
	);
}

sno_API sno_Bool sno_try_compile_source_code(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
) {
	sno_assert_ptr(state);
	sno_assert_ptr(name);
	sno_assert_ptr(source_code);

	sno_Bytecode* bytecode = sno_parse_source_code(state, name, source_code);
	if (!bytecode) {
		return sno_FALSE;
	}
	sno_Function* function = sno_create_function(state, bytecode);
	sno_Value* base = state->stack + state->stack_base;
	base->type = sno_VT_FUNCTION;
	base->v.u_function = function;
	return sno_TRUE;
}

sno_API sno_Bool sno_run_file(
	sno_State* state,
	const char* const path,
	size_t path_length
) {
	const sno_String* name = sno_create_string(state, path, path_length);
	const sno_String* file = sno_load_string_from_file(state, path, path_length);
	sno_Value* base = sno_stack_base(state);
	if (!sno_try_compile_source_code(state, name, file)) {
		return sno_FALSE;
	}
	sno_set_none(base[1]);
	sno_execute(state, 0);
}

sno_API sno_no_return void sno_throw(sno_State* state, const sno_String* exception_msg) {
	sno_assert_ptr(state);
	sno_assert_ptr(exception_msg);
	state->exception_msg = exception_msg;
	if (state->exception_jump) {
		longjmp(state->exception_jump->buf, 1);
	} else {
		sno_print_exception_msg(state);
		fputs(sno_ANSI_RED "FATAL ERROR! Uncaught Sno exception thrown! Exiting application...\n" sno_ANSI_NORMAL, stderr);
		exit(EXIT_FAILURE);
	}
}
