#include "sno_lib.h"
#include "sno_state.h"
#include "sno_gc.h"

#include <string.h>

static void print_args(sno_State* state, uint8_t num_args) {
	for (uint8_t i = 0; i < num_args; i++) {
		sno_print_value(state, &state->stack[state->stack_base + 2 + i]);
	}
}

uint8_t snol_core_print(sno_State* state, uint8_t num_args) {
	print_args(state, num_args);
	putchar('\n');
	return 0;
}

uint8_t snol_core_printnl(sno_State* state, uint8_t num_args) {
	print_args(state, num_args);
	return 0;
}

uint8_t snol_core_input(sno_State* state, uint8_t num_args) {
	print_args(state, num_args);
	char inp_str[sno_STACK_BUFFER_LENGTH] = { 0 };
	fgets(inp_str, sno_STACK_BUFFER_LENGTH - 1, stdin);
	size_t inp_len = strlen(inp_str);
	if (inp_str[inp_len - 1] == '\n') {
		inp_len--;
	}
	sno_set_ret_string(state, 0, sno_create_string(state, inp_str, inp_len));
	return 1;
}

uint8_t snol_core_typename(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 1);
	const sno_Value* item = sno_get_arg(state, 0);
	const char* const name = sno_type_strings[item->type];
	sno_set_ret_string(state, 0, sno_create_string(state, name, strlen(name)));
	return 1;
}

uint8_t snol_core_perftimer(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	sno_set_ret_number(state, 0, sno_perftimer());
	return 1;
}

uint8_t snol_core_assert(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 1);
	sno_Value* condition = sno_get_arg(state, 0);
	if (!sno_value_to_bool(condition)) {
		sno_throw_runtime_error(state, "Assertion failed");
	}
	return 0;
}

uint8_t snol_core_force_gc(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	sno_full_gc(state);
	return 0;
}

const sno_Library sno_lib_core[] = {
	sno_LibFn(core, print),
	sno_LibFn(core, printnl),
	sno_LibFn(core, input),
	sno_LibFn(core, typename),
	sno_LibFn(core, perftimer),
	sno_LibFn(core, assert),
	sno_LibFn(core, force_gc),
	sno_LibEnd
};
