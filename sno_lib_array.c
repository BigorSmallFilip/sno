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
	sno_push_back_array(state, arr, item);
	sno_set_ret_number(state, 0, arr->items.count);
	return 1;
}

uint8_t snol_array_pop(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	sno_Array* arr = sno_get_arg_typed(state, sno_VT_ARRAY, sno_self)->v.u_array;
	sno_dynarray_pop_back(state, &arr->items, sizeof(sno_Value), sno_stack_base(state));
	return 1;
}

uint8_t snol_array_has(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 1);
	sno_Array* arr = sno_get_arg_typed(state, sno_VT_ARRAY, sno_self)->v.u_array;
	sno_Value* item = sno_get_arg(state, 0);
	sno_Bool has = sno_FALSE;
	if (arr->items_type >= 0 && arr->items_type != item->type) {
		// Array is heterogenous and the checked item is a different type
		goto skip_check;
	}
	sno_Value* values = (sno_Value*)arr->items.buffer;
	for (size_t i = 0; i < arr->items.count; i++) {
		if (sno_value_equals(item, &values[i])) {
			has = sno_TRUE;
			break;
		}
	}
skip_check:
	sno_set_ret_bool(state, 0, has);
	return 1;
}

const sno_Library sno_lib_array[] = {
	sno_LibFn(array, length),
	sno_LibFn(array, push),
	sno_LibFn(array, pop),
	sno_LibFn(array, has),
	sno_LibEnd
};
