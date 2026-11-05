#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_string_to_upper(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 0);
	check_arg_type(state, 0, VT_STRING);
	IString* string = state->stack_base[0].u.string;
	state->stack_top = state->stack_base + 1;
	set_string(state->stack_base[0], istring_to_upper(state, string));
	return 1;
}

const sno_Library lib_string[] = {
	sno_Fn(string, to_upper),
	sno_FnEnd(),
};
