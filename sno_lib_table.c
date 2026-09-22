#include "sno_lib.h"
#include "sno_state.h"

uint8_t snol_table_insert(sno_State* state, uint8_t num_args) {

	putchar('\n');
	return 0;
}

uint8_t snol_table_set_prototype(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 1);
	sno_Table* table = sno_get_arg_typed(state, sno_VT_TABLE, sno_self)->v.u_table;
	sno_Table* prototype = sno_get_arg_typed(state, sno_VT_TABLE, 0)->v.u_table;
	table->prototype = prototype;
	*sno_stack_base(state) = *sno_get_arg(state, sno_self);
	return 1;
}

const sno_Library sno_lib_table[] = {
	sno_LibFn(table, insert),
	sno_LibFn(table, set_prototype),
	sno_LibEnd
};
