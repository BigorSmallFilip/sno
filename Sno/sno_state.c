#include "sno_state.h"

#include "sno_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

sno_API sno_VMState* sno_create_vm(sno_GlobalState* state) {
	sno_assert_ptr(state);
	sno_VMState* vm = state_alloc(state, sizeof(sno_VMState));
	memset(vm, 0, sizeof(sno_VMState));
	vm->state = state;
	return vm;
}

sno_API void sno_free_vm(sno_VMState* vm) {
	sno_assert_ptr(vm);
	sno_GlobalState* state = vm->state;
	sno_assert_ptr(state);

	state_free(state, sizeof(sno_VMState), vm);
}



sno_no_return void vm_throw(
	sno_VMState* vm,
	ExceptionType type,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(message);
	sno_assert_ptr(type != EXCEPTION_NONE);
	sno_assert_ptr(type < NUM_EXCEPTION_TYPES);

	vm->exception_type = type;
	vm->exception_message = create_istring(vm->state, message, message_length);
	longjmp(vm->exception_jump->buf, type);
}

sno_API sno_no_return void sno_throw_runtime_error(
	sno_VMState* vm,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(message);
	vm_throw(vm, EXCEPTION_RUNTIME_ERROR, message, message_length);
}

sno_API void sno_print_error_message(const sno_VMState* vm) {
	sno_assert_ptr(vm);
	if (vm->exception_type == EXCEPTION_NONE) {
		fprintf(stderr, "No exception\n");
	} else {
		fprintf(
			stderr,
			"%.*s\n",
			(unsigned int)vm->exception_message->length,
			istring_chars(vm->exception_message)
		);
	}
}

sno_API void sno_clear_error(sno_VMState* vm) {
	sno_assert(vm->exception_type != EXCEPTION_NONE);
	vm->exception_message = NULL;
	vm->exception_type = EXCEPTION_NONE;
}



sno_API void sno_run_test_thing(sno_GlobalState* state) {
	sno_assert_ptr(state);

	(void)create_istring(state, sno_string_comma_length("What"));
	(void)create_istring(state, sno_string_comma_length("Is"));
	(void)create_istring(state, sno_string_comma_length("Even"));
	(void)create_istring(state, sno_string_comma_length("Going"));
	(void)create_istring(state, sno_string_comma_length("On?"));
	(void)create_istring(state, sno_string_comma_length("xyz"));

	sno_VMState* vm = sno_create_vm(state);
	IString* path = create_istring(state, sno_string_comma_length("test.sno"));
	IString* source_code = load_istring_from_file(
		state,
		istring_chars(path),
		path->length
	);
	if (!source_code) {
		return;
	}

	Bytecode* bytecode = compile_source_code(vm, path, source_code);
	if (!bytecode) {
		sno_print_error_message(vm);
		sno_clear_error(vm);
	}

	(void)print_source_code_tokens(vm, path, source_code);

	//print_string_interning_table(state);
}
