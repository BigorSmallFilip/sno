#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_string_length(sno_State* state, uint8_t num_args) {
	
	putchar('\n');
	return 0;
}

const sno_Library sno_lib_string[] = {
	sno_LibFn(string, length),
	sno_LibEnd
};
