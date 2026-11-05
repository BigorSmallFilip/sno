#include "sno_state.h"
#include "sno_value.h"

#include <stdio.h>

static uint8_t lib_core_print(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 1]);
	}
	putchar('\n');
	return 0;
}

static uint8_t lib_core_printnl(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 1]);
	}
	return 0;
}

static uint8_t lib_core_ord(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 1);
	check_arg_type(state, 1, VT_STRING);
	IString* string = state->stack_base[1].u.string;
	if (string->length != 1) {
		sno_throw_runtime_error(
			state,
			"Expected argument to be a single character"
		);
	}
	state->stack_top = state->stack_base + 1;
	set_number(state->stack_top[-1], (sno_Number)istring_chars(string)[0]);
	return 1;
}

static uint8_t lib_core_chr(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 1);
	check_arg_type(state, 1, VT_NUMBER);
	sno_Number n = state->stack_base[1].u.number;
	if (!sno_number_is_valid_u8(n)) {
		sno_throw_runtime_error(
			state,
			"Expected argument to be a valid byte"
		);
	}
	uint8_t b = (uint8_t)n;
	const char* const buf = (char*)&b;
	state->stack_top = state->stack_base + 1;
	set_string(state->stack_top[-1], create_istring(state, buf, 1));
	return 1;
}

const sno_Library lib_core[] = {
	sno_Fn(core, print),
	sno_Fn(core, printnl),
	sno_Fn(core, ord),
	sno_Fn(core, chr),
	sno_FnEnd(),
};
