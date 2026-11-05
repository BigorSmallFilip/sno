#include "sno_state.h"
#include "sno_value.h"

#include <stdio.h>

static uint8_t lib_core_print(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 1]);
	}
	putchar('\n');
	set_retc(0);
}

static uint8_t lib_core_printnl(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 1]);
	}
	set_retc(0);
}

static uint8_t lib_core_input(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 1]);
	}
	char inp_str[sno_STACK_BUFFER_LENGTH] = { 0 };
	fgets(inp_str, sno_STACK_BUFFER_LENGTH - 1, stdin);
	size_t inp_len = strlen(inp_str);
	if (inp_str[inp_len - 1] == '\n') {
		inp_len--;
	}
	set_string(state->stack_base[0], create_istring(state, inp_str, inp_len));
	set_retc(1);
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
	set_number(state->stack_base[0], (sno_Number)istring_chars(string)[0]);
	set_retc(1);
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
	set_string(state->stack_base[0], create_istring(state, buf, 1));
	set_retc(1);
}

static uint8_t lib_core_perftimer(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 0);
	set_number(state->stack_base[0], perftimer());
	set_retc(1);
}

const sno_Library lib_core[] = {
	sno_Fn(core, print),
	sno_Fn(core, printnl),
	sno_Fn(core, input),
	sno_Fn(core, ord),
	sno_Fn(core, chr),
	sno_Fn(core, perftimer),
	sno_FnEnd(),
};
