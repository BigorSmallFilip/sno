#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_core_print(sno_State* state, uint8_t num_args) {
	for (uint8_t i = 0; i < num_args; i++) {
		sno_print_value(state, &state->stack[state->stack_base + 2 + i]);
	}
	putchar('\n');
	return 0;
}

uint8_t snol_core_perftimer(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	sno_set_ret_number(state, 0, sno_perftimer());
	return 1;
}

uint8_t snol_core_assert(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 1);
	sno_Value* condition = sno_get_arg(state, 0);
	if (!sno_value_to_bool(condition)) {
		sno_throw_runtime_error(state, "Assertion failed");
	}
	return 0;
}

const sno_Library sno_lib_core[] = {
	sno_LibFn(core, print),
	sno_LibFn(core, perftimer),
	sno_LibFn(core, assert),
	sno_LibEnd
};
