#include "sno_state.h"

#include "sno_compiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

sno_API sno_GlobalState* sno_create_state(void) {
	sno_GlobalState* state = malloc(sizeof(sno_GlobalState));
	if (!state) {
		return NULL;
	}
	state->mem_allocated = 0;
	state->num_allocations = 0;
	init_string_interning_table(state, 64);
	return state;
}

sno_API void sno_free_state(sno_GlobalState* state) {
	sno_assert_ptr(state);
	free(state);
}

sno_API sno_VMState* sno_create_vm(sno_GlobalState* state) {
	sno_assert_ptr(state);
	sno_VMState* vm = state_alloc(state, sizeof(sno_VMState));
	memset(vm, 0, sizeof(sno_VMState));
	vm->state = state;
	return vm;
}

sno_API void sno_free_vm(sno_VMState* vm) {
	sno_assert_ptr(vm);
	sno_GlobalState* state = vm->state;
	sno_assert_ptr(state);

	state_free(state, sizeof(sno_VMState), vm);
}



sno_no_return void vm_throw(
	sno_VMState* vm,
	ExceptionType type,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(message);
	sno_assert_ptr(type != EXCEPTION_NONE);
	sno_assert_ptr(type < NUM_EXCEPTION_TYPES);

	vm->exception_type = type;
	vm->exception_message = create_istring(vm->state, message, message_length);
	longjmp(vm->exception_jump->buf, type);
}

sno_API sno_no_return void sno_throw_runtime_error(
	sno_VMState* vm,
	const char* const message,
	size_t message_length
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(message);
	vm_throw(vm, EXCEPTION_RUNTIME_ERROR, message, message_length);
}

sno_API void sno_print_error_message(const sno_VMState* vm) {
	sno_assert_ptr(vm);
	if (vm->exception_type == EXCEPTION_NONE) {
		fprintf(stderr, "No exception\n");
	} else {
		fprintf(
			stderr,
			"%.*s\n",
			(unsigned int)vm->exception_message->length,
			istring_chars(vm->exception_message)
		);
	}
}

sno_API void sno_clear_error(sno_VMState* vm) {
	sno_assert(vm->exception_type != EXCEPTION_NONE);
	vm->exception_message = NULL;
	vm->exception_type = EXCEPTION_NONE;
}



sno_API uint8_t sno_call(sno_VMState* vm, uint8_t argc) {
	sno_assert_ptr(vm);
	sno_assert(argc <= MAX_EXPR_PER_STMT);
	return 0;
}



static sno_Bool add_str_key_str_value(
	sno_VMState* vm,
	Table* table,
	const char* const key,
	const char* const value
) {
	Value vk;
	Value vv;
	vk.type = VT_STRING;
	vk.u.string = create_istring(vm->state, key, strlen(key));
	vv.type = VT_STRING;
	vv.u.string = create_istring(vm->state, value, strlen(value));
	return table_set_key(vm, table, vk, vv);
}

static sno_Bool remove_str_key(
	sno_VMState* vm,
	Table* table,
	const char* const key
) {
	Value vk;
	vk.type = VT_STRING;
	vk.u.string = create_istring(vm->state, key, strlen(key));
	return table_remove_key(vm, table, vk);
}

static void test_tables(sno_VMState* vm) {
	Table* table = create_table(vm);
	Value table_value;
	table_value.type = VT_TABLE;
	table_value.u.table = table;
	add_str_key_str_value(vm, table, "name1", "Alice");
	add_str_key_str_value(vm, table, "name2", "Bob");
	add_str_key_str_value(vm, table, "name3", "Corn");
	add_str_key_str_value(vm, table, "name4", "Dale");
	add_str_key_str_value(vm, table, "name5", "Eagle");
	add_str_key_str_value(vm, table, "name6", "Falcone");
	add_str_key_str_value(vm, table, "name7", "Group");
	add_str_key_str_value(vm, table, "name8", "Ham");
	add_str_key_str_value(vm, table, "name9", "Insides");
	//print_value(table_value); putchar('\n');
	remove_str_key(vm, table, "name1");
	//print_value(table_value); putchar('\n');

	size_t final_count_expected = table->count;
	for (size_t j = 0; j < 100; j++) {
		for (size_t i = 0; i < 1000; i++) {
			char key[128];
			snprintf(key, 128, "%i", (int)i);
			add_str_key_str_value(vm, table, key, key);
		}
		Value nk = { .type = VT_NUMBER };
		nk.u.number = (sno_Number)j;
		final_count_expected++;
		table_set_key(vm, table, nk, nk);
		for (size_t i = 0; i < 1000; i++) {
			char key[128];
			snprintf(key, 128, "%i", (int)i);
			remove_str_key(vm, table, key);
		}
		//print_value(table_value); putchar('\n');
	}
	sno_assert(table->count == final_count_expected);
}

sno_API void sno_run_test_thing(sno_GlobalState* state) {
	sno_assert_ptr(state);

	(void)create_istring(state, sno_string_comma_length("What"));
	(void)create_istring(state, sno_string_comma_length("Is"));
	(void)create_istring(state, sno_string_comma_length("Even"));
	(void)create_istring(state, sno_string_comma_length("Going"));
	(void)create_istring(state, sno_string_comma_length("On?"));
	(void)create_istring(state, sno_string_comma_length("xyz"));

	sno_VMState* vm = sno_create_vm(state);
	IString* path = create_istring(state, sno_string_comma_length("test.sno"));
	IString* source_code = load_istring_from_file(
		state,
		istring_chars(path),
		path->length
	);
	if (!source_code) {
		return;
	}

	Bytecode* bytecode = compile_source_code(vm, path, source_code);
	if (!bytecode) {
		sno_print_error_message(vm);
		sno_clear_error(vm);
	}

	(void)print_source_code_tokens(vm, path, source_code);

	//test_tables(vm);
	print_string_interning_table(state);
}
