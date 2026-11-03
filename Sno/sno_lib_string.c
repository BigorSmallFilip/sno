#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_string_to_upper(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 0);
	return 0;
}

const sno_Library lib_string[] = {
	sno_Fn(string, to_upper),
	sno_FnEnd(),
};
