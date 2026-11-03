#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_array_push(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 1);
	return 0;
}

const sno_Library lib_array[] = {
	sno_Fn(array, push),
	sno_FnEnd(),
};
