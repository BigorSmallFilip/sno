#include "sno_state.h"

#include "sno_mem.h"
#include "sno_parser.h"
#include "sno_vm.h"
#include "sno_lib.h"
#include <stdarg.h>

static void init_stack(sno_State* state, uint32_t capacity);

static void sno_load_core_libs(sno_State* state) {
	sno_load_lib_into_global_scope(state, sno_lib_core);
	sno_load_lib_into_global_scope(state, sno_lib_math);
	state->string_prototype = sno_load_lib_into_table(state, sno_lib_string);
	state->array_prototype = sno_load_lib_into_table(state, sno_lib_array);
	state->table_prototype = sno_load_lib_into_table(state, sno_lib_table);
}

sno_API sno_State* sno_create_state() {
	sno_State* state = sno_alloc_type(NULL, sno_State);
	sno_init_string_interning_table(state, 64);
	init_stack(state, 128);
	sno_load_core_libs(state);
	return state;
}

static void init_stack(sno_State* state, uint32_t capacity) {
	sno_assert_ptr(state);
	sno_assert_msg(capacity != 0, "Capcity must not be 0");
	sno_assert_msg(sno_is_power_of_2(capacity), "Capcity must be power of 2");
	sno_assert(capacity <= sno_MAX_STACK);

	state->stack = sno_malloc(state, capacity * sizeof(sno_Value));
	sno_assert_ptr(state->stack);
	state->stack_capacity = capacity;
	state->stack_base = 0;
	state->stack_top = 0;

	state->globals = sno_create_table(state, 64);
}

static void resize_stack(sno_State* state, uint32_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->stack);
	sno_assert(state->stack_capacity > 0);
	sno_assert_msg(new_capacity != 0, "Capcity must not be 0");
	sno_assert_msg(sno_is_power_of_2(new_capacity), "Capcity must be power of 2");
	sno_assert(new_capacity <= sno_MAX_STACK);

	state->stack = sno_realloc(state, state->stack, new_capacity * sizeof(sno_Value));
	sno_assert_ptr(state->stack);
	state->stack_capacity = new_capacity;
}

sno_API void sno_reserve_stack(sno_State* state, uint32_t slots) {
	state->stack_top += slots;
	if (state->stack_top > state->stack_capacity) {
		resize_stack(state, state->stack_capacity << 1);
	}
}

sno_API void sno_free_state(sno_State* state) {
	if (!state) {
		return;
	}
}



sno_API const sno_Value* sno_get_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	return &sno_arg(arg);
}

sno_API sno_Bool sno_get_bool_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	sno_Value value = sno_arg(arg);
	if (value.type != sno_VT_BOOL) {
		sno_throw_runtime_error(state, "BAD ARGUMENT AAAAAAAAA");
	}
	return value.v.u_number != sno_NUMBER_FALSE;
}

sno_API sno_Number sno_get_number_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	sno_Value value = sno_arg(arg);
	if (value.type != sno_VT_NUMBER) {
		sno_throw_runtime_error(state, "BAD ARGUMENT AAAAAAAAA");
	}
	return value.v.u_number;
}

sno_API const sno_String* sno_get_string_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	sno_Value value = sno_arg(arg);
	if (value.type != sno_VT_STRING) {
		sno_throw_runtime_error(state, "BAD ARGUMENT AAAAAAAAA");
	}
	return value.v.u_string;
}

sno_API sno_Array* sno_get_array_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	sno_Value value = sno_arg(arg);
	if (value.type != sno_VT_ARRAY) {
		sno_throw_runtime_error(state, "BAD ARGUMENT AAAAAAAAA");
	}
	return value.v.u_array;
}

sno_API sno_Table* sno_get_table_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	sno_Value value = sno_arg(arg);
	if (value.type != sno_VT_TABLE) {
		sno_throw_runtime_error(state, "BAD ARGUMENT AAAAAAAAA");
	}
	return value.v.u_table;
}

sno_API sno_Function* sno_get_function_arg(sno_State* state, int arg) {
	sno_assert(arg >= -1 && arg < sno_MAX_STACK_ARGS);
	sno_Value value = sno_arg(arg);
	if (value.type != sno_VT_FUNCTION) {
		sno_throw_runtime_error(state, "BAD ARGUMENT AAAAAAAAA");
	}
	return value.v.u_function;
}



sno_API void sno_create_new_global(sno_State* state, const sno_String* name, const sno_Value* value) {
	sno_Value key;
	key.type = sno_VT_STRING;
	key.v.u_string = name;
	if (sno_table_set_or_add_key(state, state->globals, &key, value)) {
		sno_throw_runtime_error(
			state,
			"A global variable named '%.*s' already exists",
			name->length,
			sno_string_chars(name)
		);
	}
}

sno_API void sno_set_global(sno_State* state, const sno_String* name, const sno_Value* value) {
	sno_Value key;
	key.type = sno_VT_STRING;
	key.v.u_string = name;
	if (!sno_table_set(state->globals, &key, value)) {
		sno_throw_runtime_error(
			state,
			"Couldn't find global variable '%.*s'",
			name->length,
			sno_string_chars(name)
		);
	}
}

sno_API void sno_get_global(sno_State* state, const sno_String* name, const sno_Value* out_value) {
	sno_Value key;
	key.type = sno_VT_STRING;
	key.v.u_string = name;
	if (!sno_table_set(state->globals, &key, out_value)) {
		sno_throw_runtime_error(
			state,
			"Couldn't find global variable '%.*s'",
			name->length,
			sno_string_chars(name)
		);
	}
}



sno_API void sno_print_globals(const sno_State* state) {
	sno_assert_ptr(state);

}

sno_API void sno_print_exception_msg(const sno_State* state) {
	sno_assert_ptr(state);
	fprintf(
		stderr,
		"%.*s\n",
		(unsigned int)state->exception_msg->length,
		sno_string_chars(state->exception_msg)
	);
}

sno_API sno_Bool sno_try_compile_source_code(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
) {
	sno_assert_ptr(state);
	sno_assert_ptr(name);
	sno_assert_ptr(source_code);

	sno_Bytecode* bytecode = sno_parse_source_code(state, name, source_code);
	if (!bytecode) {
		return sno_FALSE;
	}
	sno_Function* function = sno_create_function(state, bytecode);
	sno_Value* base = state->stack + state->stack_base;
	base->type = sno_VT_FUNCTION;
	base->v.u_function = function;
	return sno_TRUE;
}

sno_API void sno_call(sno_State* state, uint8_t num_args, uint8_t num_returns) {
	sno_assert_ptr(state);
	sno_assert(num_args < sno_MAX_STACK_ARGS);
	sno_assert(num_returns < sno_MAX_STACK_ARGS);

	sno_Value* base = sno_stack_base(state);
	if (base->type != sno_VT_FUNCTION) {
		sno_throw_runtime_error(
			state,
			"Tried to call something which wasn't a function"
		);
	}
	sno_Function* function = base->v.u_function;
	uint8_t num_real_returns = 0;
	if (function->is_c_function) {
		sno_CFunction* c_function = function->u.c_function;
		num_real_returns = c_function(state, num_args);
	} else {
		num_real_returns = sno_execute(state, num_args);
	}
	for (uint8_t i = num_real_returns; i < num_returns; i++) {
		sno_set_none(base[i]);
	}
}

sno_API sno_Bool sno_run_file(
	sno_State* state,
	const char* const path,
	size_t path_length
) {
	const sno_String* name = sno_create_string(state, path, path_length);
	const sno_String* file = sno_load_string_from_file(state, path, path_length);
	sno_Value* base = sno_stack_base(state);
	if (!sno_try_compile_source_code(state, name, file)) {
		return sno_FALSE;
	}
	sno_set_none(base[1]);
	sno_execute(state, 0);
	return sno_TRUE;
}

sno_API sno_no_return void sno_throw(sno_State* state, const sno_String* exception_msg) {
	sno_assert_ptr(state);
	sno_assert_ptr(exception_msg);
	state->exception_msg = exception_msg;
	if (state->exception_jump) {
		longjmp(state->exception_jump->buf, 1);
	} else {
		sno_print_exception_msg(state);
		fputs(sno_ANSI_RED "FATAL ERROR! Uncaught Sno exception thrown! Exiting application...\n" sno_ANSI_NORMAL, stderr);
		exit(EXIT_FAILURE);
	}
}
