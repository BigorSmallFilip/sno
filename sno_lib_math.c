#include "sno_lib.h"
#include "sno_state.h"

#include "sno_math.h"

#define MATH(func) \
	uint8_t snol_math_##func(sno_State* state, uint8_t num_args) { \
		sno_check_arg_count(state, num_args, 1); \
		sno_set_ret_number(state, 0, func(sno_get_number_arg(state, 0))); \
		return 1; \
	}

MATH(sin);
MATH(cos);
MATH(tan);
MATH(asin);
MATH(acos);
MATH(atan);
MATH(sinh);
MATH(cosh);
MATH(tanh);
MATH(asinh);
MATH(acosh);
MATH(atanh);

MATH(floor);
MATH(ceil);
MATH(round);

MATH(sqrt);



uint8_t snol_math_lerp(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 3);
	sno_set_ret_number(state, 0, sno_lerp(
		sno_get_number_arg(state, 0),
		sno_get_number_arg(state, 1),
		sno_get_number_arg(state, 2)
	));
	return 1;
}



static uint32_t random_uint32_t(sno_State* state) {
	uint64_t oldseed = state->random_seed;
	state->random_seed = oldseed * 0x5851F42D4C957F2Dull + 0xDA3E39CB94B95BDBull;
	uint32_t xorshifted = (uint32_t)(((oldseed >> 18) ^ oldseed) >> 27);
	uint32_t rot = (uint32_t)(oldseed >> 59);
	return (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31));
}

static sno_Number random_range(sno_State* state, sno_Number min, sno_Number max) {
	sno_Number t = (sno_Number)random_uint32_t(state) / (sno_Number)UINT32_MAX;
	return sno_lerp(min, max, t);
}


uint8_t snol_math_random(sno_State* state, uint8_t num_args) {
	sno_Number min = 0;
	sno_Number max = 1;
	if (num_args == 1) {
		max = sno_get_number_arg(state, 0);
	} else if (num_args == 2) {
		min = sno_get_number_arg(state, 0);
		max = sno_get_number_arg(state, 1);
	} else if (num_args > 2) {
		sno_throw_runtime_error(state, "Too many arguments");
	}
	sno_Number r = random_range(state, min, max);
	sno_set_ret_number(state, 0, r);
	return 1;
}

const sno_Library sno_lib_math[] = {
	sno_LibFn(math, sin),
	sno_LibFn(math, cos),
	sno_LibFn(math, tan),
	sno_LibFn(math, asin),
	sno_LibFn(math, acos),
	sno_LibFn(math, atan),
	sno_LibFn(math, sinh),
	sno_LibFn(math, cosh),
	sno_LibFn(math, tanh),
	sno_LibFn(math, asinh),
	sno_LibFn(math, acosh),
	sno_LibFn(math, atanh),
	sno_LibFn(math, floor),
	sno_LibFn(math, ceil),
	sno_LibFn(math, round),
	sno_LibFn(math, sqrt),
	sno_LibFn(math, random),
	sno_LibEnd
};
