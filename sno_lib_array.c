#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_array_push(sno_State* state, uint8_t num_args) {
	
	return 1;
}

const sno_Library sno_lib_array[] = {
	sno_LibFn(array, push),
	sno_LibEnd
};
