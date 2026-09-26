#include "sno_value.h"

#include "sno_state.h"
#include "sno_vm.h"
#include "sno_parser.h"
#include "sno_gc.h"
#include <string.h>



const char* const sno_type_strings[sno_NUM_VALUE_TYPES] = {
	"none",
	"bool",
	"number",
	"(linalg)",
	"string",
	"array",
	"table",
	"function",
};

const char* const sno_type_strings_noun[sno_NUM_VALUE_TYPES] = {
	"none",
	"a bool",
	"a number",
	"a (linalg)",
	"a string",
	"an array",
	"a table",
	"a function",
};



const char* const sno_linalg_type_strings[sno_NUM_LINEAR_ALGEBRA_TYPES] = {
	"vec2",
	"vec3",
	"vec4",
	"quat",
	"mat2",
	"mat3",
	"mat4",
};

const char* const sno_linalg_type_strings_noun[sno_NUM_LINEAR_ALGEBRA_TYPES] = {
	"a vec2",
	"a vec3",
	"a vec4",
	"a quat",
	"a mat2",
	"a mat3",
	"a mat4",
};

const uint8_t sno_linalg_type_length[sno_NUM_LINEAR_ALGEBRA_TYPES] = {
	2, 3, 4, 4, 4, 9, 16,
};
const uint8_t sno_linalg_type_num_rows[sno_NUM_LINEAR_ALGEBRA_TYPES] = {
	2, 3, 4, 4, 2, 3, 4,
};
const uint8_t sno_linalg_type_num_columns[sno_NUM_LINEAR_ALGEBRA_TYPES] = {
	1, 1, 1, 1, 2, 3, 4,
};



const char* const sno_get_type_string(const sno_Value* v) {
	sno_assert_ptr(v);
	sno_assert(v->type < sno_NUM_VALUE_TYPES);
	if (v->type == sno_VT_LINALG) {
		sno_LinAlgType linalg_type = v->v.u_linalg->la_type;
		sno_assert(linalg_type < sno_NUM_LINEAR_ALGEBRA_TYPES);
		return sno_linalg_type_strings[linalg_type];
	}
	return sno_type_strings[v->type];
}

const char* const sno_get_type_string_noun(const sno_Value* v) {
	sno_assert_ptr(v);
	sno_assert(v->type < sno_NUM_VALUE_TYPES);
	if (v->type == sno_VT_LINALG) {
		sno_LinAlgType linalg_type = v->v.u_linalg->la_type;
		sno_assert(linalg_type < sno_NUM_LINEAR_ALGEBRA_TYPES);
		return sno_linalg_type_strings_noun[linalg_type];
	}
	return sno_type_strings_noun[v->type];
}



sno_LinAlg* sno_create_linalg(sno_State* state, sno_LinAlgType type) {
	sno_assert_ptr(state);
	sno_assert(type >= 0 && type < sno_NUM_LINEAR_ALGEBRA_TYPES);
	sno_LinAlg* linalg = sno_calloc(
		state,
		1,
		sizeof(sno_LinAlg) +
		sno_linalg_type_length[type] * sizeof(sno_Number)
	);
	linalg->la_type = type;
	linalg->gc_mark = sno_GC_MARK_DEAD;
	linalg->gc_type = sno_OT_LINALG;
	linalg->gc_next = state->gc_list_start;
	state->gc_list_start = (sno_GCObject*)linalg;
	state->num_non_string_gc_objects++;
	return linalg;
}



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
	arr->items_type = sno_HT_UNDEFINED;
	state->num_non_string_gc_objects++;
	state->gc_list_start = (sno_GCObject*)arr;
	return arr;
}

void sno_push_back_array(sno_State* state, sno_Array* arr, sno_Value* item) {
	if (arr->items_type == sno_HT_UNDEFINED) {
		arr->items_type = item->type;
	} else if (arr->items_type != item->type) {
		arr->items_type = sno_HT_MULTIPLE;
	}
	sno_dynarray_push_back(state, &arr->items, sizeof(sno_Value), item);
}

void sno_concat_array(sno_State* state, sno_Array* arr, sno_Value* items, size_t count) {
	sno_assert(count != 0);

	size_t i = 0;
	if (arr->items_type == sno_HT_UNDEFINED) {
		arr->items_type = items[0].type;
		i++;
	}
	for (; i < count; i++) {
		if (items[i].type != arr->items_type) {
			arr->items_type = sno_HT_MULTIPLE;
			break;
		}
	}

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
	state->num_non_string_gc_objects++;
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
	sno_assert(obj->gc_type != sno_OT_STRING);
	if (prev) {
		sno_assert(prev->gc_next == obj);
		prev->gc_next = obj->gc_next;
	} else {
		sno_assert(state->gc_list_start == obj);
		state->gc_list_start = obj->gc_next;
	}
	state->num_non_string_gc_objects--;
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
	case sno_OT_FUNCTION: {
		sno_free(state, obj, sizeof(sno_Function));
	} break;
	case sno_OT_BYTECODE: {
		sno_Bytecode* bytecode = (sno_Bytecode*)obj;
		sno_free_bytecode(state, bytecode);
	} break;
	default:
		break;
	}
}



static void print_number(sno_State* state, sno_Number number) {
	if (sno_number_is_valid_i64(number)) {
		printf("%lli", (int64_t)number);
	} else {
		printf("%g", number);
	}
}

static void print_linalg(sno_State* state, const sno_LinAlg* linalg) {
	sno_assert_ptr(state);
	sno_assert_ptr(linalg);
	sno_assert(linalg->la_type <= sno_NUM_LINEAR_ALGEBRA_TYPES);
	printf("%s(", sno_linalg_type_strings[linalg->la_type]);
	const size_t length = sno_linalg_type_length[linalg->la_type];
	size_t i = 0;
	while (1) {
		print_number(state, linalg->components[i]);
		i++;
		if (i < length) {
			printf(", ");
		} else {
			printf(")");
			return;
		}
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
	size_t items_to_print = sno_min(10, arr->items.count);
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
	case sno_VT_NUMBER: print_number(state, v->v.u_number); break;
	case sno_VT_LINALG: print_linalg(state, v->v.u_linalg); break;
	case sno_VT_STRING: printf("%.*s", (unsigned int)v->v.u_string->length, sno_string_chars(v->v.u_string)); break;
	case sno_VT_ARRAY: print_array(state, v->v.u_array); break;
	case sno_VT_TABLE: print_table(state, v->v.u_table); break;
	case sno_VT_FUNCTION: {
		if (v->v.u_function->is_c_function) {
			printf("C function 0x%p", v->v.u_function);
		} else {
			printf(
				"Sno function 0x%p \"%.*s\"",
				v->v.u_function,
				(unsigned int)   v->v.u_function->u.bytecode->name->length,
				sno_string_chars(v->v.u_function->u.bytecode->name)
			);
		}
	} break;
	default: printf("0x%p", v->v.u_ptr); break;
	}
}



static void number_to_string(
	sno_State* state,
	sno_DynArray* string,
	sno_Number number
) {
	char buf[sno_STACK_BUFFER_LENGTH];
	int len;
	if (sno_number_is_valid_i64(number)) {
		len = snprintf(buf, sno_STACK_BUFFER_LENGTH - 1, "%lli", (uint64_t)number);
	} else {
		len = snprintf(buf, sno_STACK_BUFFER_LENGTH - 1, "%g", number);
	}
	sno_dynarray_push_back_bytes(state, string, buf, len);
}

static void linalg_to_string(
	sno_State* state,
	sno_DynArray* string,
	const sno_LinAlg* linalg
) {
	const sno_LinAlgType type = linalg->la_type;
	sno_assert(strlen(sno_linalg_type_strings[type]) == 4);
	sno_dynarray_push_back_bytes(state, string, sno_linalg_type_strings[type], 4);
	sno_dynarray_push_back_bytes(state, string, sno_str_comma_len("("));
	size_t i = 0;
	while (1) {
		number_to_string(state, string, linalg->components[i]);
		i++;
		if (i < sno_linalg_type_length[type]) {
			sno_dynarray_push_back_bytes(state, string, sno_str_comma_len(", "));
		} else {
			sno_dynarray_push_back_bytes(state, string, sno_str_comma_len(")"));
			break;
		}
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
		number_to_string(state, string, v->v.u_number);
	} break;
	case sno_VT_LINALG: {
		linalg_to_string(state, string, v->v.u_linalg);
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
		sno_not_implemented;
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
	sno_dynarray_clear(state, &stringbuf, 1);
	return istring;
}



static sno_Bool linalg_eq(const sno_LinAlg* a, const sno_LinAlg* b) {
	sno_assert(a->la_type < sno_NUM_LINEAR_ALGEBRA_TYPES);
	if (a->la_type != b->la_type) return sno_FALSE;
	const size_t length = sno_linalg_type_length[a->la_type];
	for (size_t i = 0; i < length; i++) {
		if (a->components[i] != b->components[i]) {
			return sno_FALSE;
		}
	}
	return sno_TRUE;
}

sno_Bool sno_value_equals(const sno_Value* a, const sno_Value* b) {
	if (a->type != b->type) return sno_FALSE;
	if (a->type == sno_VT_NONE) return sno_TRUE;
	if (a->type == sno_VT_NUMBER) return a->v.u_number == b->v.u_number;
	if (a->type == sno_VT_LINALG) return linalg_eq(a->v.u_linalg, b->v.u_linalg);
	return a->v.u_data == b->v.u_data;
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
	function->gc_mark = 0;
	function->gc_type = sno_OT_FUNCTION;
	function->gc_next = state->gc_list_start;
	state->gc_list_start = (sno_GCObject*)function;
	state->num_non_string_gc_objects++;

	function->is_c_function = sno_FALSE;
	function->u.bytecode = bytecode;
	return function;
}
