#include "sno_state.h"
#include "sno_value.h"

static uint8_t lib_table_insert(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_check_argc(state, argc, 2);
	return 0;
}

const sno_Library lib_table[] = {
	sno_Fn(table, insert),
	sno_FnEnd(),
};
