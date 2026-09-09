#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_array_push(sno_State* state, uint8_t num_args) {
	sno_Array* arr = sno_get_array_arg(state, sno_self);
	sno_Value* item = sno_get_arg(state, 0);
	sno_dynarray_push_back(state, &arr->items, sizeof(sno_Value), item);
	sno_set_number(sno_ret(0), arr->items.count);
	return 1;
}

const sno_Library sno_lib_array[] = {
	sno_LibFn(array, push),
	sno_LibEnd
};
