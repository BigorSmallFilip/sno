#include "sno_value.h"

#include "sno_state.h"
#include "sno_vm.h"
#include "sno_parser.h"
#include "sno_gc.h"
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



sno_Array* sno_create_array(sno_State* state, size_t capacity) {
	sno_assert_ptr(state);
	sno_assert(sno_is_power_of_2(capacity));
	sno_assert(capacity < sno_SIZE_T_LIMIT);
	sno_assert(capacity >= 4);
	sno_Array* arr = sno_alloc_type(state, sno_Array);
	sno_assert_ptr(arr);
	sno_dynarray_init(state, &arr->items, sizeof(sno_Value), capacity);
	arr->gc_mark = sno_GC_MARK_DEAD;
	arr->gc_type = sno_OT_ARRAY;
	arr->gc_next = state->gc_list_start;
	state->num_gc_objects++;
	state->gc_list_start = (sno_GCObject*)arr;
	return arr;
}

void sno_concat_array(sno_State* state, sno_Array* arr, sno_Value* items, size_t count) {
	sno_assert(count != 0);
	sno_dynarray_reserve(state, &arr->items, sizeof(sno_Value), count);
	memcpy(((sno_Value*)arr->items.buffer) + arr->items.count, items, sizeof(sno_Value) * count);
	arr->items.count += count;
}



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
	table->gc_mark = sno_GC_MARK_DEAD;
	table->gc_type = sno_OT_TABLE;
	table->gc_next = state->gc_list_start;
	state->num_gc_objects++;
	state->gc_list_start = (sno_GCObject*)table;
	sno_assert_ptr(table->nodes);
	return table;
}

static sno_Bool key_equals(const sno_TableNode* node, const sno_Value* key) {
	sno_assert_ptr(node);
	sno_assert_ptr(key);
	if (node->key_type != key->type) return sno_FALSE;
	switch (key->type) {
	case sno_VT_NONE: return sno_TRUE;
	case sno_VT_BOOL:
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
				sno_free(state, node, sizeof(sno_TableNode));
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



void sno_table_iter(sno_Table* table, size_t* bucket, sno_TableNode** node) {
	sno_assert_ptr(table);
	sno_assert_ptr(bucket);
	sno_assert_ptr(node);
	for (*bucket = 0; *bucket < table->capacity_mask + 1; (*bucket)++) {
		*node = &table->nodes[*bucket];
		if ((*node)->exists) {
			return;
		}
	}
	*node = NULL;
}

void sno_table_next(sno_Table* table, size_t* bucket, sno_TableNode** node) {
	sno_assert_ptr(table);
	sno_assert_ptr(bucket);
	sno_assert_ptr(node);
	sno_assert_ptr(*node);
	*node = (*node)->next;
	if (!(*node)) {
		// No node left in the linked list
		// Search in the next buckets
		(*bucket)++;
		for (; *bucket < table->capacity_mask + 1; (*bucket)++) {
			*node = &table->nodes[*bucket];
			if ((*node)->exists) {
				return;
			}
		}
		*node = NULL;
	}
}

static void clear_table(sno_State* state, sno_Table* table) {
	for (size_t i = 0; i < table->capacity_mask + 1; i++) {
		sno_TableNode* node = &table->nodes[i];
		if (node->exists) {
			node = node->next; // Don't free the nodes in the array, only the linked ones after
			while (node) {
				sno_TableNode* next = node->next;
				sno_free(state, node, sizeof(sno_TableNode));
				node = next;
			}
		}
	}
	sno_free(state, table->nodes, (table->capacity_mask + 1) * sizeof(sno_TableNode));
}



void sno_free_gc_object(
	sno_State* state,
	sno_GCObject* obj,
	sno_GCObject* prev
) {
	if (prev) {
		sno_assert(prev->gc_next == obj);
		prev->gc_next = obj->gc_next;
	} else {
		sno_assert(state->gc_list_start == obj);
		state->gc_list_start = obj->gc_next;
	}
	state->num_gc_objects--;
	//printf("Freeing GC object. Now there are %u\n", state->num_gc_objects);
	
	switch (obj->gc_type) {
	case sno_OT_ARRAY: {
		sno_Array* arr = (sno_Array*)obj;
		sno_dynarray_clear(state, &arr->items, sizeof(sno_Value));
		sno_free(state, obj, sizeof(sno_Array));
	} break;
	case sno_OT_TABLE: {
		sno_Table* table = (sno_Table*)obj;
		clear_table(state, table);
		sno_free(state, obj, sizeof(sno_Table));
	} break;
	default:
		break;
	}
}



static void print_array(sno_State* state, const sno_Array* arr) {
	sno_assert_ptr(state);
	sno_assert_ptr(arr);
	if (arr->items.count == 0) {
		printf("[]");
		return;
	}
	printf("[");
	size_t items_to_print = min(10, arr->items.count);
	for (size_t i = 0; i < items_to_print; i++) {
		if (i == 10 - 1) {
			printf(", ... ");
			break;
		}
		if (i != 0) {
			printf(", ");
		}
		const sno_Value* v = sno_dynarray_get(state, &arr->items, sizeof(sno_Value), i);
		sno_print_value(state, v);
	}
	printf("]");
}

static void print_table(sno_State* state, const sno_Table* table) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	if (table->count == 0) {
		printf("{}");
		return;
	}
	printf("{");
	size_t bucket = 0;
	sno_TableNode* node = NULL;
	sno_Bool first = sno_TRUE;
	sno_table_iter(table, &bucket, &node);
	while (node) {
		sno_Value key;
		key.type = node->key_type;
		key.v.u_data = node->key_union.u_data;
		sno_Value value;
		value.type = node->value_type;
		value.v.u_data = node->value_union.u_data;
		if (!first) {
			printf(", ");
		}
		sno_print_value(state, &key);
		printf(": ");
		sno_print_value(state, &value);
		sno_table_next(table, &bucket, &node);
		first = sno_FALSE;
	}
	printf("}");
}

void sno_print_value(sno_State* state, const sno_Value* v) {
	switch (v->type) {
	case sno_VT_NONE: printf("none"); break;
	case sno_VT_BOOL: printf(v->v.u_number != 0 ? "true" : "false"); break;
	case sno_VT_NUMBER: {
		sno_Number n = v->v.u_number;
		if (sno_number_is_valid_i64(n)) {
			printf("%lli", (int64_t)n);
		} else {
			printf("%g", v->v.u_number);
		}
	} break;
	case sno_VT_STRING: printf("%.*s", (unsigned int)v->v.u_string->length, sno_string_chars(v->v.u_string)); break;
	case sno_VT_ARRAY: print_array(state, v->v.u_array); break;
	case sno_VT_TABLE: print_table(state, v->v.u_table); break;
	case sno_VT_FUNCTION: printf("function 0x%p", v->v.u_function); break;
	default: printf("0x%p", v->v.u_ptr); break;
	}
}



static void array_to_string(
	sno_State* state,
	sno_DynArray* string,
	const sno_Array* arr,
	size_t recursion_limit
) {
	if (recursion_limit == -1) {
		sno_dynarray_push_back_bytes(state, string, sno_str_comma_len("[...]"));
		return;
	}
	sno_Value* values = (sno_Value*)arr->items.buffer;
	sno_dynarray_push_back_bytes(state, string, sno_str_comma_len("["));
	for (size_t i = 0; i < arr->items.count; i++) {
		sno_value_to_string(state, string, &values[i], recursion_limit);
		if (i != arr->items.count - 1) {
			sno_dynarray_push_back_bytes(state, string, sno_str_comma_len(", "));
		}
	}
	sno_dynarray_push_back_bytes(state, string, sno_str_comma_len("]"));
}

void sno_value_to_string(
	sno_State* state,
	sno_DynArray* string,
	const sno_Value* v,
	size_t recursion_limit
) {
	switch (v->type) {
	case sno_VT_NONE: {
		sno_dynarray_push_back_bytes(state, string,
			sno_str_comma_len("none")
		);
	} break;
	case sno_VT_BOOL: {
		if (v->v.u_number) {
			sno_dynarray_push_back_bytes(state, string,
				sno_str_comma_len("true")
			);
		} else {
			sno_dynarray_push_back_bytes(state, string,
				sno_str_comma_len("false")
			);
		}
	} break;
	case sno_VT_NUMBER: {
		sno_Number n = v->v.u_number;
		char buf[sno_STACK_BUFFER_LENGTH];
		int len;
		if (sno_number_is_valid_i64(n)) {
			len = snprintf(buf, sno_STACK_BUFFER_LENGTH - 1, "%lli", (uint64_t)n);
		} else {
			len = snprintf(buf, sno_STACK_BUFFER_LENGTH - 1, "%g", n);
		}
		sno_dynarray_push_back_bytes(state, string, buf, len);
	} break;
	case sno_VT_STRING: {
		sno_dynarray_push_back_bytes(state, string,
			sno_string_chars(v->v.u_string),
			v->v.u_string->length
		);
	} break;
	case sno_VT_ARRAY: {
		array_to_string(state, string, v->v.u_array, recursion_limit - 1);
	} break;
	default:
		break;
	}
}

const sno_IString* sno_interpolate_string(
	sno_State* state,
	const sno_Value* values,
	uint32_t num_values
) {
	sno_DynArray stringbuf;
	sno_dynarray_init(state, &stringbuf, 1, 256);
	for (uint32_t i = 0; i < num_values; i++) {
		sno_value_to_string(
			state,
			&stringbuf,
			&values[i],
			4
		);
	}
	const sno_IString* istring = sno_create_string(
		state,
		(const char*)stringbuf.buffer,
		stringbuf.count
	);
	return istring;
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
	case sno_VT_BOOL: value->v.u_number ? 1 : 2;
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
	function->gc_next = state->gc_list_start;
	function->gc_type = sno_OT_FUNCTION;
	function->is_c_function = sno_FALSE;
	function->u.bytecode = bytecode;
	return function;
}
