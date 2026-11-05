#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_table_insert(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 2);
	check_arg_type(state, 0, VT_TABLE);
	Table* table = state->stack_base[0].u.table;
	Value key = state->stack_base[1];
	Value value = state->stack_base[2];
	state->stack_top = state->stack_base + 1;
	set_bool(state->stack_top[-1], table_set_or_add_key(
		state, table, key, value
	));
	return 1;
}

static uint8_t lib_table_has(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 1);
	check_arg_type(state, 0, VT_TABLE);
	Table* table = state->stack_base[0].u.table;
	Value key = state->stack_base[1];
	state->stack_top = state->stack_base + 1;
	set_bool(state->stack_top[-1], table_has_key(table, key));
	return 1;
}

const sno_Library lib_table[] = {
	sno_Fn(table, insert),
	sno_Fn(table, has),
	sno_FnEnd(),
};
