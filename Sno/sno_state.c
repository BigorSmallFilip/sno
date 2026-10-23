#include "sno_state.h"

#include "sno_compiler.h"
#include "sno_lib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

DEFINE_GENERIC_DYN_ARRAY(CallInfo, CallInfo, call_info);



static void init_stack(sno_State* state);

sno_API sno_State* sno_create_state(void) {
	sno_State* state = malloc(sizeof(sno_State));
	if (!state) {
		return NULL;
	}
	memset(state, 0, sizeof(sno_State));
	if (!state) {
		return NULL;
	}
	state->mem_allocated = 0;
	state->num_allocations = 0;
	init_string_interning_set(state, 64);
	init_stack(state);
	state->globals = create_table(state);
	sno_load_library_into_global_scope(state, lib_core);
	return state;
}

sno_API void sno_free_state(sno_State* state) {
	sno_assert_ptr(state);
	free(state);
}



static void init_stack(sno_State* state) {
	sno_assert_ptr(state);
	state->stack = state_alloc(state, sizeof(Value) * INITIAL_STACK_SIZE);
	state->stack_base = state->stack;
	state->stack_top = state->stack;
	state->stack_capacity = INITIAL_STACK_SIZE;
}

static void resize_stack(sno_State* state, uint32_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert(sno_is_power_of_2(new_capacity));
	sno_assert(new_capacity >= 64 && new_capacity < sno_MAX_STACK);
	sno_assert(new_capacity > state->stack_capacity);
	uint32_t stack_base_idx = (uint32_t)(state->stack_base - state->stack);
	uint32_t stack_top_idx = (uint32_t)(state->stack_top - state->stack);
	state->stack = state_realloc(
		state,
		sizeof(Value) * state->stack_capacity,
		state->stack,
		sizeof(Value) * new_capacity
	);
	state->stack_base = state->stack + stack_base_idx;
	state->stack_top = state->stack + stack_top_idx;
	state->stack_capacity = new_capacity;
}

sno_API void sno_reserve_stack(sno_State* state, uint32_t slots) {
	sno_assert_ptr(state);
	uint32_t stack_top_idx = (uint32_t)(state->stack_top - state->stack);
	uint32_t capacity_needed = stack_top_idx + slots;
	if (capacity_needed >= sno_MAX_STACK) {
		sno_throw_runtime_error(
			state,
			sno_string_comma_length(
				"API stack too big. Likely due to recursion"
			)
		);
	}
	if (capacity_needed > state->stack_capacity) {
		resize_stack(state, (uint32_t)pow2_ge((uint32_t)capacity_needed));
	}
	state->stack_top += slots;
	sno_assert(state->stack_top - state->stack <= state->stack_capacity);
}

sno_API void sno_copy(sno_State* state, uint32_t idx_from, uint32_t idx_to) {
	sno_assert_ptr(state);
	sno_assert(idx_from < state->stack_top - state->stack_base);
	sno_assert(idx_to < state->stack_top - state->stack_base);
	sno_assert(idx_from != idx_to);
	state->stack_base[idx_to] = state->stack_base[idx_from];
}

sno_API void sno_set_none(sno_State* state, uint32_t idx) {
	sno_assert_ptr(state);
	sno_assert(idx < state->stack_top - state->stack_base);
	memset(&state->stack_base[idx], 0, sizeof(Value));
}

sno_API void sno_set_bool(sno_State* state, uint32_t idx, sno_Bool b) {
	sno_assert_ptr(state);
	sno_assert(idx < state->stack_top - state->stack_base);
	state->stack_base[idx].type = VT_BOOL;
	state->stack_base[idx].u.number = b ? NUMBER_TRUE : NUMBER_FALSE;
}

sno_API void sno_set_number(sno_State* state, uint32_t idx, sno_Number n) {
	sno_assert_ptr(state);
	sno_assert(idx < state->stack_top - state->stack_base);
	state->stack_base[idx].type = VT_NUMBER;
	state->stack_base[idx].u.number = n;
}

sno_API void sno_create_string(
	sno_State* state,
	uint32_t idx,
	const char* const string,
	size_t length
) {
	sno_assert_ptr(state);
	sno_assert(idx < state->stack_top - state->stack_base);
	state->stack_base[idx].type = VT_STRING;
	state->stack_base[idx].u.string = create_istring(
		state,
		string,
		length
	);
}

sno_API sno_Bool sno_load_string_from_file(sno_State* state, uint32_t idx) {
	sno_assert_ptr(state);
	sno_assert(idx < state->stack_top - state->stack_base);
	sno_assert(state->stack_base[idx].type == VT_STRING);
	IString* path = state->stack_base[idx].u.string;
	IString* string = load_istring_from_file(
		state,
		istring_chars(path),
		path->length
	);
	if (!string) {
		sno_set_none(state, idx);
		return sno_FALSE;
	} else {
		state->stack_base[idx].u.string = string;
		return sno_TRUE;
	}
}





void create_new_global(sno_State* state, Value name, Value value) {
	sno_assert_ptr(state);
	sno_assert(name.type == VT_STRING);
	if (table_set_or_add_key(state, state->globals, name, value)) {
		sno_throw_runtime_error(
			state,
			"A global variable named \"%.*s\" already exists",
			(unsigned int)name.u.string->length,
			istring_chars(name.u.string)
		);
	}
}



sno_no_return void state_throw(
	sno_State* state,
	ExceptionType type,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(state);
	sno_assert_ptr(message);
	sno_assert_ptr(type != EXCEPTION_NONE);
	sno_assert_ptr(type < NUM_EXCEPTION_TYPES);

	state->exception_type = type;
	state->exception_message = create_istring(state, message, message_length);
	if (state->exception_jump) {
		longjmp(state->exception_jump->buf, type);
	} else {
		fprintf(
			stderr,
			sno_ANSI_RED "Uncaught Sno exception\n%.*s" sno_ANSI_NORMAL,
			(unsigned int)message_length,
			message
		);
		exit(EXIT_FAILURE);
	}
}

sno_API sno_no_return void sno_throw_runtime_error(
	sno_State* state,
	const char* const format,
	...
) {
	sno_assert_ptr(state);
	sno_assert_ptr(format);
	char msg[sno_STACK_BUFFER_LENGTH];
	size_t length = 0;
	va_list args;
	va_start(args, format);
	length += (size_t)vsnprintf(
		msg + length,
		sno_STACK_BUFFER_LENGTH - length,
		format,
		args
	);
	va_end(args);
	state_throw(state, EXCEPTION_RUNTIME_ERROR, msg, length);
}

sno_API void sno_print_error_message(const sno_State* state) {
	sno_assert_ptr(state);
	if (state->exception_type == EXCEPTION_NONE) {
		fprintf(stderr, "No exception\n");
	} else {
		fprintf(
			stderr,
			"%.*s\n",
			(unsigned int)state->exception_message->length,
			istring_chars(state->exception_message)
		);
	}
}

sno_API void sno_clear_error(sno_State* state) {
	sno_assert_ptr(state);
	sno_assert(state->exception_type != EXCEPTION_NONE);
	state->exception_message = NULL;
	state->exception_type = EXCEPTION_NONE;
}

sno_API sno_Bool sno_compile(sno_State* state) {
	sno_assert_ptr(state);
	sno_assert_msg(
		state->stack_top - state->stack >= 2,
		"Too many arguments underflowing the stack"
	);
	// Two string args
	sno_assert(state->stack_top[-1].type == VT_STRING);
	sno_assert(state->stack_top[-2].type == VT_STRING);
	Bytecode* bytecode = compile_source_code(
		state,
		state->stack_top[-2].u.string,
		state->stack_top[-1].u.string
	);
	// Remove two string args, push one return value
	state->stack_top += (-2) + (1);
	if (!bytecode) {
		sno_set_none(state, 0);
		return sno_FALSE;
	}
	state->stack_top[-1].type = VT_BYTECODE;
	state->stack_top[-1].u.bytecode = bytecode;
	return sno_TRUE;
}



sno_API uint8_t sno_call(sno_State* state, uint8_t argc, uint8_t retc) {
	sno_assert_ptr(state);
	sno_assert(argc <= MAX_EXPR_PER_STMT);
	sno_assert_msg(
		state->stack_top - state->stack >= argc + 2,
		"Too many arguments underflowing the stack"
	);
	Value* func_value = state->stack_top - (argc + 2);
	sno_assert_msg(
		func_value->type == VT_BYTECODE ||
		func_value->type == VT_FOREIGN_FUNCTION,
		"Tried to call something which wasn't a function"
	);
	CallInfo call_info;
	uint8_t retc_found;
	call_info.caller_base_idx = (uint32_t)(state->stack_base - state->stack);
	state->stack_base = state->stack_top - (argc + 2);
	call_info.argc = argc;
	if (func_value->type == VT_BYTECODE) {
		call_info.is_foreign_function = sno_FALSE;
		call_info.function.bytecode = func_value->u.bytecode;
		call_info_dyn_array_push(state, &state->call_stack, &call_info);
		retc_found = execute_bytecode(state, argc);
		for (size_t i = 0; i < retc; i++) {
			if (i >= retc_found) {
				state->stack_base[i].type = VT_NONE;
			} else {
				state->stack_base[i] = state->stack_top[-retc_found + i];
			}
		}
		state->stack_top = state->stack_base + retc;
		state->stack_base = state->stack + call_info.caller_base_idx;
	} else {
		call_info.is_foreign_function = sno_TRUE;
		call_info.function.foreign = func_value->u.foreign_function;
		call_info_dyn_array_push(state, &state->call_stack, &call_info);
		sno_ForeignFunction* ff = func_value->u.foreign_function;
		retc_found = ff(state, argc);
		state->stack_base = state->stack + call_info.caller_base_idx;
	}
	
	state->call_stack.count--; // Pop the call info
	return retc_found;
}



static sno_Bool add_str_key_str_value(
	sno_State* state,
	Table* table,
	const char* const key,
	const char* const value
) {
	Value vk;
	Value vv;
	vk.type = VT_STRING;
	vk.u.string = create_istring(state, key, strlen(key));
	vv.type = VT_STRING;
	vv.u.string = create_istring(state, value, strlen(value));
	return table_set_or_add_key(state, table, vk, vv);
}

static sno_Bool remove_str_key(
	sno_State* state,
	Table* table,
	const char* const key
) {
	Value vk;
	vk.type = VT_STRING;
	vk.u.string = create_istring(state, key, strlen(key));
	return table_remove_key(state, table, vk);
}

static void test_tables(sno_State* state) {
	Table* table = create_table(state);
	Value table_value;
	table_value.type = VT_TABLE;
	table_value.u.table = table;
	add_str_key_str_value(state, table, "name1", "Alice");
	add_str_key_str_value(state, table, "name2", "Bob");
	add_str_key_str_value(state, table, "name3", "Corn");
	add_str_key_str_value(state, table, "name4", "Dale");
	add_str_key_str_value(state, table, "name5", "Eagle");
	add_str_key_str_value(state, table, "name6", "Falcone");
	add_str_key_str_value(state, table, "name7", "Group");
	add_str_key_str_value(state, table, "name8", "Ham");
	add_str_key_str_value(state, table, "name9", "Insides");
	//print_value(table_value); putchar('\n');
	remove_str_key(state, table, "name1");
	//print_value(table_value); putchar('\n');

	size_t final_count_expected = table->count;
	for (size_t j = 0; j < 100; j++) {
		for (size_t i = 0; i < 1000; i++) {
			char key[128];
			snprintf(key, 128, "%i", (int)i);
			add_str_key_str_value(state, table, key, key);
		}
		Value nk = { .type = VT_NUMBER };
		nk.u.number = (sno_Number)j;
		final_count_expected++;
		table_set_or_add_key(state, table, nk, nk);
		for (size_t i = 0; i < 1000; i++) {
			char key[128];
			snprintf(key, 128, "%i", (int)i);
			remove_str_key(state, table, key);
		}
		//print_value(table_value); putchar('\n');
	}
	sno_assert(table->count == final_count_expected);
}



sno_API void sno_load_library_into_global_scope(
	sno_State* state,
	const sno_Library* library
) {
	sno_assert_ptr(state);
	sno_assert_ptr(library);
	int i = 0;
	while (1) {
		const sno_Library* lib_func = &library[i];
		if (!lib_func->foreign_function) {
			break;
		}
		Value name;
		name.type = VT_STRING;
		name.u.string = create_istring(
			state,
			lib_func->name,
			lib_func->name_length
		);
		Value v;
		v.type = VT_FOREIGN_FUNCTION;
		v.u.foreign_function = lib_func->foreign_function;
		create_new_global(state, name, v);
		i++;
	}
	return;
}





static void test_string_set(sno_State* state) {
	for (size_t i = 0; i < 1000; i++) {
		char str[128];
		int length = snprintf(str, 128, "%i", (int)i);
		create_istring(state, str, (size_t)length);
	}
}

sno_API void sno_run_test_thing(sno_State* state) {
	sno_assert_ptr(state);

	(void)create_istring(state, sno_string_comma_length("What"));
	(void)create_istring(state, sno_string_comma_length("Is"));
	(void)create_istring(state, sno_string_comma_length("Even"));
	(void)create_istring(state, sno_string_comma_length("Going"));
	(void)create_istring(state, sno_string_comma_length("On?"));
	(void)create_istring(state, sno_string_comma_length("xyz"));

	IString* path = create_istring(state, sno_string_comma_length("test.sno"));
	IString* source_code = load_istring_from_file(
		state,
		istring_chars(path),
		path->length
	);
	if (!source_code) {
		return;
	}

	Bytecode* bytecode = compile_source_code(state, path, source_code);
	if (!bytecode) {
		sno_print_error_message(state);
		sno_clear_error(state);
	}

	(void)print_source_code_tokens(state, path, source_code);

	test_string_set(state);
	//test_tables(state);
	print_string_interning_set(state);
}
