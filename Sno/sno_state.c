#include "sno_state.h"

#include "sno_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>



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
	init_string_interning_table(state, 64);
	init_stack(state);
	return state;
}

sno_API void sno_free_state(sno_State* state) {
	sno_assert_ptr(state);
	free(state);
}



static void init_stack(sno_State* state) {
	sno_assert_ptr(state);
	state->stack = state_alloc(state, sizeof(Value) * 128);
	state->stack_capacity = 128;
	state->stack_top = state->stack;
}

static void resize_stack(sno_State* state, uint32_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert(sno_is_power_of_2(new_capacity));
	sno_assert(new_capacity >= 64 && new_capacity < sno_MAX_STACK);
	sno_assert(new_capacity > state->stack_capacity);
	uint32_t stack_top_idx = (uint32_t)(state->stack_top - state->stack);
	state->stack = state_realloc(
		state,
		sizeof(Value) * state->stack_capacity,
		state->stack,
		sizeof(Value) * new_capacity
	);
	state->stack_capacity = new_capacity;
	state->stack_top = state->stack + stack_top_idx;
}

sno_API void sno_reserve_stack(sno_State* state, uint32_t slots) {
	sno_assert_ptr(state);
	
}

sno_API void sno_set_none(sno_State* state, uint32_t idx) {
	return sno_API void();
}

sno_API void sno_set_bool(sno_State* state, uint32_t idx, sno_Bool b) {
	return sno_API void();
}

sno_API void sno_set_number(sno_State* state, uint32_t idx, sno_Number n) {
	return sno_API void();
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
	longjmp(state->exception_jump->buf, type);
}

sno_API sno_no_return void sno_throw_runtime_error(
	sno_State* state,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(state);
	sno_assert_ptr(message);
	state_throw(state, EXCEPTION_RUNTIME_ERROR, message, message_length);
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
	sno_assert(state->exception_type != EXCEPTION_NONE);
	state->exception_message = NULL;
	state->exception_type = EXCEPTION_NONE;
}



sno_API uint8_t sno_call(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_assert(argc <= MAX_EXPR_PER_STMT);
	return 0;
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
	return table_set_key(state, table, vk, vv);
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
		table_set_key(state, table, nk, nk);
		for (size_t i = 0; i < 1000; i++) {
			char key[128];
			snprintf(key, 128, "%i", (int)i);
			remove_str_key(state, table, key);
		}
		//print_value(table_value); putchar('\n');
	}
	sno_assert(table->count == final_count_expected);
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

	//test_tables(state);
	print_string_interning_table(state);
}
