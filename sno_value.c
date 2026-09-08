#include "sno_value.h"

#include "sno_state.h"
#include "sno_vm.h"
#include <string.h>



const char* const sno_type_strings[7] = {
	"none",
	"bool",
	"number",
	"string",
	"array",
	"table",
	"function",
};

const char* const sno_type_strings_noun[7] = {
	"none",
	"a bool",
	"a number",
	"a string",
	"an array",
	"a table",
	"a function",
};



sno_Table* sno_create_table(sno_State* state, size_t capacity) {
	sno_assert_ptr(state);
	sno_assert(sno_is_power_of_2(capacity));
	sno_assert(capacity < sno_SIZE_T_LIMIT);
	sno_assert(capacity >= 8);
	sno_Table* table = sno_alloc_type(state, sno_Table);
	sno_assert_ptr(table);
	table->count = 0;
	table->capacity_mask = capacity - 1;
	table->nodes = sno_calloc(state, capacity, sizeof(sno_TableNode));
	sno_assert_ptr(table->nodes);
	return table;
}

static sno_Bool key_equals(const sno_TableNode* node, const sno_Value* key) {
	sno_assert_ptr(node);
	sno_assert_ptr(key);
	if (node->key_type != key->type) return sno_FALSE;
	switch (key->type) {
	case sno_VT_NONE: return sno_TRUE;
	case sno_VT_NUMBER: return key->v.u_number == node->key_union.u_number;
	case sno_VT_STRING: return key->v.u_string == node->key_union.u_string;
	case sno_VT_ARRAY: return key->v.u_array == node->key_union.u_array;
	case sno_VT_TABLE: return key->v.u_table == node->key_union.u_table;
	case sno_VT_FUNCTION: return key->v.u_function == node->key_union.u_function;
	}
	sno_unreachable;
}

static sno_TableNode* get_table_node(const sno_Table* table, const sno_Value* key) {
	sno_assert_ptr(table);
	sno_assert_ptr(key);
	uint32_t hashmod = sno_hash_value(key) & table->capacity_mask;
	sno_TableNode* node = &table->nodes[hashmod];
	if (!node->exists) {
		return NULL;
	}
	while (node) {
		if (key_equals(node, key)) { return node; }
		node = node->next;
	}
	return NULL;
}

static sno_TableNode* get_or_add_new_table_node(sno_State* state, const sno_Table* table, const sno_Value* key) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	sno_assert_ptr(key);
	uint32_t hashmod = sno_hash_value(key) & table->capacity_mask;
	sno_TableNode* node = &table->nodes[hashmod];
	if (!node->exists) {
		return node;
	}
	while (node) {
		if (key_equals(node, key)) { return node; }
		if (!node->next) { break; }
		node = node->next;
	}
	sno_TableNode* new_linked_node = sno_alloc_type(state, sno_TableNode);
	new_linked_node->next = NULL;
	new_linked_node->exists = sno_FALSE;
	node->next = new_linked_node;
	return new_linked_node;
}

static sno_Bool remove_table_node(sno_State* state, sno_Table* table, const sno_Value* key) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	uint32_t hashmod = sno_hash_value(key) & table->capacity_mask;
	sno_TableNode* node = &table->nodes[hashmod];
	if (!node->exists) {
		return sno_FALSE;
	}
	sno_TableNode* prev = NULL;
	while (node) {
		if (key_equals(node, key)) {
			node->exists = sno_FALSE;
			if (prev) {
				prev->next = node->next;
				sno_free(state, node);
			}
			table->count--;
			sno_assert(table->count >= 0);
			return sno_TRUE;
		}
		prev = node;
		node = node->next;
	}
	return sno_FALSE;
}

sno_Bool sno_table_set_or_add_key(sno_State* state, sno_Table* table, const sno_Value* key, const sno_Value* value) {
	sno_TableNode* node = get_or_add_new_table_node(state, table, key);
	node->key_type = key->type;
	node->key_union.u_data = key->v.u_data;
	node->value_type = value->type;
	node->value_union.u_data = value->v.u_data;
	if (node->exists) {
		return sno_TRUE;
	}
	node->exists = sno_TRUE;
	table->count++;
	return sno_FALSE;
}

sno_Bool sno_table_set(sno_Table* table, const sno_Value* key, const sno_Value* value) {
	sno_TableNode* node = get_table_node(table, key);
	if (!node) return sno_FALSE;
	node->key_type = key->type;
	node->key_union.u_data = key->v.u_data;
	node->value_type = value->type;
	node->value_union.u_data = value->v.u_data;
	node->exists = sno_TRUE;
	return sno_TRUE;
}

sno_Bool sno_table_get(sno_Table* table, const sno_Value* key, sno_Value* out_value) {
	sno_assert_ptr(table);
	sno_assert_ptr(key);
	sno_assert_ptr(out_value);
	sno_TableNode* node = get_table_node(table, key);
	if (!node) {
		out_value->type = sno_VT_NONE;
		out_value->v.u_data = 0;
		return sno_FALSE;
	}
	sno_assert(node->key_type == key->type);
	sno_assert(node->key_union.u_data == key->v.u_data);
	out_value->type = node->value_type;
	out_value->v.u_data = node->value_union.u_data;
	return sno_TRUE;
}

sno_Bool sno_table_has(sno_Table* table, const sno_Value* key) {
	sno_assert_ptr(table);
	sno_assert_ptr(key);
	sno_TableNode* node = get_table_node(table, key);
	return node != NULL;
}

sno_Bool sno_table_remove(sno_State* state, sno_Table* table, const sno_Value* key) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	sno_assert_ptr(key);
	return remove_table_node(state, table, key);
}



void sno_print_value(const sno_Value* v) {
	switch (v->type) {
	case sno_VT_NONE: printf("none"); break;
	case sno_VT_BOOL: printf(v->v.u_number != 0 ? "true" : "false"); break;
	case sno_VT_NUMBER: printf("%g", v->v.u_number); break;
	case sno_VT_STRING: printf("%.*s", (unsigned int)v->v.u_string->length, sno_string_chars(v->v.u_string)); break;
	//case sno_VT_ARRAY: sno_PrintArray(v->v.u_array); break;
	//case sno_VT_TABLE: sno_PrintTable(v->v.u_table); break;
	default: printf("0x%p", v->v.u_ptr); break;
	}
}

sno_Bool sno_value_equals(const sno_Value* a, const sno_Value* b) {
	if (a->type != b->type) return sno_FALSE;
	if (a->type == sno_VT_NONE) return sno_TRUE;
	if (a->type == sno_VT_NUMBER) return a->v.u_number == b->v.u_number;
	else return a->v.u_data == b->v.u_data;
}

sno_Bool sno_value_to_bool(const sno_Value* v) {
	if (v->type == sno_VT_NUMBER) {
		return v->v.u_number != 0.0;
	} else {
		return v->v.u_data != 0;
	}
}

static sno_Hash hash_number(sno_Number number) {
	sno_Hash ints[sizeof(sno_Number) / sizeof(sno_Hash)];
	sno_assert(sizeof(ints) == sizeof(sno_Number));
	int i;
	if (number == 0.0) {
		return 0;
	}
	memcpy(ints, &number, sizeof(ints));
	for (i = 1; i < sizeof(sno_Number) / sizeof(sno_Hash); i++) {
		ints[0] += ints[i];
	}
	return ints[0];
}

static sno_Hash hash_pointer(void* ptr) {
	return (sno_Hash)((uintptr_t)ptr >> 3);
}

sno_Hash sno_hash_value(const sno_Value* value) {
	switch (value->type) {
	case sno_VT_NONE: return 0;
	case sno_VT_NUMBER: return hash_number(value->v.u_number);
	case sno_VT_STRING: return value->v.u_string->hash;
	case sno_VT_ARRAY:
	case sno_VT_TABLE:
	case sno_VT_FUNCTION: return hash_pointer(value->v.u_array);
	}
	sno_unreachable;
	return 0;
}

sno_Function* sno_create_function(sno_State* state, const sno_Bytecode* bytecode) {
	sno_Function* function = sno_alloc_type(state, sno_Function);
	function->is_c_function = sno_FALSE;
	function->u.bytecode = bytecode;
	return function;
}
