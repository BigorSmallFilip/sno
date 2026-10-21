#include "sno_lib.h"

#include "sno_state.h"
#include "sno_value.h"

uint8_t core_print(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	for (size_t i = 0; i < argc; i++) {
		print_value(state->stack_base[i + 2]);
	}
	return 0;
}

const sno_Library lib_core[] = {
	sno_Fn(core, print),
	sno_FnEnd(),
};