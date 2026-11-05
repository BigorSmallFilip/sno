#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_array_push(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 1);
	check_arg_type(state, 0, VT_ARRAY);
	Array* arr = state->stack_base[0].u.arr;
	array_push(state, arr, state->stack_base[1]);
	state->stack_top = state->stack_base + 1;
	set_number(state->stack_top[-1], (sno_Number)arr->values.count);
	return 1;
}

static uint8_t lib_array_pop(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 0);
	check_arg_type(state, 0, VT_ARRAY);
	Array* arr = state->stack_base[0].u.arr;
	state->stack_top = state->stack_base + 1;
	state->stack_base[-1] = array_pop(state, arr);
	return 1;
}

const sno_Library lib_array[] = {
	sno_Fn(array, push),
	sno_Fn(array, pop),
	sno_FnEnd(),
};
