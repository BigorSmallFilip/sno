#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_array_push(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 1);
	check_arg_type(state, 0, VT_ARRAY);
	Array* arr = state->stack_base[0].u.arr;
	array_push(state, arr, state->stack_base[1]);
	return 0;
}

const sno_Library lib_array[] = {
	sno_Fn(array, push),
	sno_FnEnd(),
};
