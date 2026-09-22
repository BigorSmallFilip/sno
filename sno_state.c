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
	sno_State* state = calloc(1, sizeof(sno_State));
	if (!state) {
		return NULL;
	}
	sno_init_string_interning_table(state, 64);
	init_stack(state, 128);
	sno_load_core_libs(state);
	sno_dynarray_init(state, &state->call_infos, sizeof(sno_CallInfo), 16);
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
	if (new_capacity <= sno_MAX_STACK) {
		sno_throw_runtime_error(state, "Tried to make the stack really really big");
	}

	state->stack = sno_realloc(state, state->stack, state->stack_capacity, new_capacity * sizeof(sno_Value));
	sno_assert_ptr(state->stack);
	state->stack_capacity = new_capacity;
}

sno_API void sno_free_state(sno_State* state) {
	if (!state) {
		return;
	}
}





sno_API sno_Value* sno_reserve_stack(sno_State* state, uint32_t slots) {
	state->stack_top += slots;
	if (state->stack_top > state->stack_capacity) {
		resize_stack(state, state->stack_capacity << 1);
	}
	return sno_stack_base(state);
}

sno_API sno_Value* sno_get_stack_ptr(sno_State* state, uint32_t slot) {
	sno_assert_ptr(state);
	sno_assert(state->stack_base + slot < state->stack_top);
	return &state->stack[state->stack_base + slot];
}

sno_API void sno_check_arg_count(
	sno_State* state,
	uint8_t num_args,
	uint8_t num_args_expected
) {
	sno_assert_ptr(state);
	if (num_args != num_args_expected) {
		sno_throw_runtime_error(
			state,
			"Expected %u args, but was given %u",
			num_args_expected,
			num_args
		);
	}
}

sno_API sno_Value* sno_get_arg(sno_State* state, int arg) {
	return sno_get_stack_ptr(state, arg + 2);
}

const char* const arg_names[] = {
	"self", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	"10", "11", "12", "13",
};

sno_API sno_Value* sno_get_arg_typed(
	sno_State* state,
	sno_ValueType expected_type,
	int arg
) {
	sno_assert_ptr(state);
	sno_assert(arg >= sno_self && arg < sno_MAX_STACK_ARGS);
	sno_Value* value = sno_get_stack_ptr(state, arg + 2);
	if (value->type != expected_type) {
		sno_throw_runtime_error(
			state,
			"Expected %s%s to be %s, but it was %s",
			arg == sno_self ? "" : "arg ",
			arg_names[arg + 1],
			sno_type_strings_noun[expected_type],
			sno_type_strings_noun[value->type]
		);
	}
	return value;
}

sno_API void sno_set_ret_number(sno_State* state, int ret, sno_Number number) {
	sno_assert_ptr(state);
	sno_assert(ret >= 0 && ret < sno_MAX_STACK_ARGS);
	sno_Value* value = sno_get_stack_ptr(state, ret);
	sno_set_number(*value, number);
}

sno_API void sno_set_ret_bool(sno_State* state, int ret, sno_Bool b) {
	sno_assert_ptr(state);
	sno_assert(ret >= 0 && ret < sno_MAX_STACK_ARGS);
	sno_Value* value = sno_get_stack_ptr(state, ret);
	sno_set_bool(*value, b);
}



sno_API void sno_s_array_push(sno_State* state, uint32_t i) {
	
}





sno_API void sno_create_new_global(sno_State* state, const sno_IString* name, const sno_Value* value) {
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

sno_API void sno_set_global(sno_State* state, const sno_IString* name, const sno_Value* value) {
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

sno_API void sno_get_global(sno_State* state, const sno_IString* name, const sno_Value* out_value) {
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
	if (!state->exception_msg) { return; }
	fprintf(
		stderr,
		"%.*s\n",
		(unsigned int)state->exception_msg->length,
		sno_string_chars(state->exception_msg)
	);
}

sno_API sno_Bool sno_try_compile_source_code(
	sno_State* state,
	const sno_IString* name,
	const sno_IString* source_code
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

	sno_CallInfo call_info = {
		state->stack_base,
		num_args,
		num_returns,
		NULL,
	};
	sno_dynarray_push_back(
		state,
		&state->call_infos,
		sizeof(sno_CallInfo),
		&call_info
	);

	sno_Value* base = sno_stack_base(state);
	if (base->type != sno_VT_FUNCTION) {
		sno_throw_runtime_error(
			state,
			"Tried to call something which wasn't a function"
		);
	}
	sno_Function* function = base->v.u_function;
	uint8_t num_real_returns;
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
	const sno_IString* name = sno_create_string(state, path, path_length);
	const sno_IString* file = sno_load_string_from_file(state, path, path_length);
	if (!name || !file) {
		return sno_FALSE;
	}
	sno_Value* base = sno_stack_base(state);
	if (!sno_try_compile_source_code(state, name, file)) {
		return sno_FALSE;
	}
	sno_set_none(base[1]);
	sno_execute(state, 0);
	return sno_TRUE;
}





sno_API sno_no_return void sno_throw(
	sno_State* state,
	sno_ExceptionType exception_type,
	const char* const exception_msg,
	size_t exception_msg_len
) {
	sno_assert_ptr(state);
	sno_assert_ptr(exception_msg);
	state->exception_msg = sno_create_string(state, exception_msg, exception_msg_len);
	state->exception_type = exception_type;
	if (state->exception_jump) {
		longjmp(state->exception_jump->buf, 1);
	} else {
		sno_print_exception_msg(state);
		fputs(sno_ANSI_RED "FATAL ERROR! Uncaught Sno exception thrown! Exiting application...\n" sno_ANSI_NORMAL, stderr);
		exit(EXIT_FAILURE);
	}
}

sno_API sno_no_return void sno_throw_runtime_error(
	sno_State* state,
	const char* const format,
	...
) {
	va_list args;
	va_start(args, format);
	sno_throw_runtime_error_va(state, format, args);
}

sno_API sno_no_return void sno_throw_runtime_error_va(
	sno_State* state,
	const char* const format,
	va_list args
) {
	char buffer[sno_STACK_BUFFER_LENGTH];
	int length = vsnprintf(
		buffer,
		sno_STACK_BUFFER_LENGTH - 1,
		format,
		args
	);
	length += snprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		sno_ANSI_NORMAL
	);
	va_end(args);
	sno_throw(state, sno_EXCEPTION_RUNTIME_ERROR, buffer, length);
}

sno_API sno_no_return void sno_throw_at_source_code_pos(
	sno_State* state,
	sno_ExceptionType exception_type,
	const sno_IString* source_code,
	const sno_IString* source_code_name,
	uint32_t source_code_pos,
	const char* const format,
	va_list args
) {
	char buffer[sno_STACK_BUFFER_LENGTH];
	int length = 0;
	length += snprintf(
		buffer,
		sno_STACK_BUFFER_LENGTH - 1,
		"Some kind of error!\n"
	);
	length += sno_sprintf_source_code_pos(
		state,
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		source_code,
		source_code_pos
	);
	length += vsnprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		format,
		args
	);
	length += snprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		sno_ANSI_NORMAL
	);
	sno_throw(state, exception_type, buffer, (size_t)length);
}

sno_API sno_no_return void sno_throw_at_source_code_pos_open_close(
	sno_State* state,
	sno_ExceptionType exception_type,
	const sno_IString* source_code,
	const sno_IString* source_code_name,
	uint32_t source_code_pos_open,
	uint32_t source_code_pos_close,
	const char* const format,
	va_list args
) {
	char buffer[sno_STACK_BUFFER_LENGTH];
	int length = 0;
	length += snprintf(
		buffer,
		sno_STACK_BUFFER_LENGTH - 1,
		"Some kind of error!\n"
	);
	length += sno_sprintf_source_code_pos(
		state,
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		source_code,
		source_code_pos_open
	);
	//va_start(args, format);
	length += vsnprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		format,
		args
	);
	length += snprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		sno_ANSI_NORMAL
	);
	//va_end(args);
	sno_throw(state, exception_type, buffer, length);
}



#ifdef sno_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
double sno_perftimer() {
	uint64_t time = 0;
	QueryPerformanceCounter((LARGE_INTEGER*)&time);
	return (double)time / 10000000.0;
}

#else

double sno_perftimer() {
	return 0.0;
}

#endif