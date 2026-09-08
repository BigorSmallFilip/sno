#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_math_floor(sno_State* state, uint8_t num_args) {

	putchar('\n');
	return 0;
}

const sno_Library sno_lib_math[] = {
	sno_LibFn(math, floor),
	sno_LibEnd
};
