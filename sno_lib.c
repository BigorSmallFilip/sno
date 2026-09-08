#include "sno_lib.h"

#include "sno_state.h"

void sno_load_lib_into_global_scope(sno_State* state, const sno_Library* lib) {
	int i = 0;
	while (1) {
		const sno_Library* lib_func = &lib[i];
		if (!lib_func->function) {
			break;
		}
		sno_Function* function = sno_alloc_type(state, sno_Function);
		function->is_c_function = sno_TRUE;
		function->u.c_function = lib_func->function;
		sno_Value val;
		val.type = sno_VT_FUNCTION;
		val.v.u_function = function;
		sno_create_new_global(
			state,
			sno_create_string(state, lib_func->name, lib_func->name_length),
			&val
		);
		i++;
	}
}

sno_Table* sno_load_lib_into_table(sno_State* state, const sno_Library* lib) {
	sno_Table* table = sno_create_table(state, 8);
	int i = 0;
	while (1) {
		const sno_Library* lib_func = &lib[i];
		if (!lib_func->function) {
			break;
		}
		sno_Value key;
		key.type = sno_VT_STRING;
		key.v.u_string = sno_create_string(state, lib_func->name, lib_func->name_length);
		sno_Function* function = sno_alloc_type(state, sno_Function);
		function->is_c_function = sno_TRUE;
		function->u.c_function = lib_func->function;
		sno_Value val;
		val.type = sno_VT_FUNCTION;
		val.v.u_function = function;
		sno_table_set_or_add_key(state, table, &key, &val);
		i++;
	}
	return table;
}
