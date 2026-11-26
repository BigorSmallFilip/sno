#include "sno_state.h"
#include "sno_value.h"
#include "sno_math.h"

static uint8_t lib_math_random(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc_range(state, argc, 0, 2);
	sno_Number range_min = 0;
	sno_Number range_max = 1;
	if (argc == 2) {
		range_max = sno_check_number(state, 1);
	} else if (argc == 3) {
		range_min = sno_check_number(state, 1);
		range_max = sno_check_number(state, 2);
	}
	set_number(
		state->stack_base[0],
		lerp(range_min, range_max, random_number(state))
	);
	set_retc(1);
}

const sno_Library lib_math[] = {
	sno_Fn(math, random),
	sno_FnEnd(),
};
