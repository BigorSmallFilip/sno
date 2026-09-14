#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_core_print(sno_State* state, uint8_t num_args) {
	for (uint8_t i = 0; i < num_args; i++) {
		sno_print_value(state, &state->stack[state->stack_base + 2 + i]);
	}
	putchar('\n');
	return 0;
}

const sno_Library sno_lib_core[] = {
	sno_LibFn(core, print),
	sno_LibEnd
};
