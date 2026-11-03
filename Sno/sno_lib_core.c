#include "sno_state.h"
#include "sno_value.h"

#include <stdio.h>

static uint8_t lib_core_print(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 1]);
	}
	putchar('\n');
	return 0;
}

const sno_Library lib_core[] = {
	sno_Fn(core, print),
	sno_FnEnd(),
};
