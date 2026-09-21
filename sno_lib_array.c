#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_array_length(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	sno_Array* arr = sno_get_arg_typed(state, sno_VT_ARRAY, sno_self)->v.u_array;
	sno_set_ret_number(state, 0, arr->items.count);
	return 1;
}

uint8_t snol_array_push(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 1);
	sno_Array* arr = sno_get_arg_typed(state, sno_VT_ARRAY, sno_self)->v.u_array;
	sno_Value* item = sno_get_arg(state, 0);
	sno_dynarray_push_back(state, &arr->items, sizeof(sno_Value), item);
	sno_set_ret_number(state, 0, arr->items.count);
	return 1;
}

uint8_t snol_array_pop(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	sno_Array* arr = sno_get_arg_typed(state, sno_VT_ARRAY, sno_self)->v.u_array;
	sno_dynarray_pop_back(state, &arr->items, sizeof(sno_Value), sno_stack_base(state));
	return 1;
}

const sno_Library sno_lib_array[] = {
	sno_LibFn(array, length),
	sno_LibFn(array, push),
	sno_LibFn(array, pop),
	sno_LibEnd
};
