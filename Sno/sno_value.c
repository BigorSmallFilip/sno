#include "sno_value.h"

#include "sno_state.h"

#include <stdio.h>

DEFINE_GENERIC_DYN_ARRAY(Value, Value, value);



static void print_number(sno_Number number) {
	if (sno_number_is_valid_i64(number)) {
		printf("%lli", (int64_t)number);
	} else {
		printf("%g", number);
	}
}

static void print_array(Array* arr) {
	sno_assert_ptr(arr);
	printf("[");
	for (size_t i = 0; i < arr->values.count; i++) {
		print_value(arr->values.buffer[i]);
		if (i < arr->values.count - 1) {
			printf(", ");
		}
	}
	printf("]");
}

static void print_table(Table* table) {
	sno_assert_ptr(table);
	sno_Bool first_entry = sno_TRUE;
	printf("{");
	if (table->capacity_mask > 0) {
		for (uint32_t i = 0; i < table->capacity_mask + 1; i++) {
			TableEntry* entry = &table->entries[i];
			if (entry->key_type < 0) {
				continue;
			}
			if (first_entry) {
				first_entry = sno_FALSE;
			} else {
				printf(", ");
			}
			Value entry_key;
			Value entry_value;
			entry_key.type = entry->key_type;
			entry_key.u = entry->key_union;
			entry_value.type = entry->value_type;
			entry_value.u = entry->value_union;
			print_value(entry_key);
			printf(": ");
			print_value(entry_value);
		}
	}
	printf("}");
}

void print_value(Value value) {
	sno_assert(value_type_is_valid(value.type));
	switch (value.type) {
	case VT_NONE: printf("none"); return;
	case VT_BOOL: printf(
		"%s",
		value.u.number != 0.0 ? "true" : "false"
	); return;
	case VT_NUMBER: print_number(value.u.number); return;
	case VT_STRING: printf(
		"%.*s",
		(unsigned int)value.u.string->length,
		istring_chars(value.u.string)
	); return;
	case VT_ARRAY: print_array(value.u.arr); return;
	case VT_TABLE: print_table(value.u.table); return;
	default:
		sno_not_implemented;
		break;
	}
}



Array* create_array(sno_State* state) {
	sno_assert_ptr(state);
	return NULL;
}

Table* create_table(sno_State* state) {
	sno_assert_ptr(state);
	Table* table = state_alloc(state, sizeof(Table));
	memset(table, 0, sizeof(Table));
	
	return table;
}



static inline Hash hash_bits(uint64_t hash) {
	// From Wren's hashBits() which in turn cites:
	// From v8's ComputeLongHash() which in turn cites:
	// Thomas Wang, Integer Hash Functions.
	// http://www.concentric.net/~Ttwang/tech/inthash.htm
	hash = ~hash + (hash << 18);  // hash = (hash << 18) - hash - 1;
	hash = hash ^ (hash >> 31);
	hash = hash * 21;  // hash = (hash + (hash << 2)) + (hash << 4);
	hash = hash ^ (hash >> 11);
	hash = hash + (hash << 6);
	hash = hash ^ (hash >> 22);
	return (Hash)(hash & 0x3fffffff);
}

static inline Hash hash_number(sno_Number number) {
	// Hash the raw bits of the value.
	return hash_bits(*(uint64_t*)&number);
}

static Hash hash_value(Value value) {
	switch (value.type) {
	case VT_NONE: return 0;
	case VT_BOOL: value.u.number ? 1 : 2;
	case VT_NUMBER: return hash_number(value.u.number);
	case VT_STRING: return value.u.string->hash;
	case VT_ARRAY:
	case VT_TABLE:
	case VT_BYTECODE:
	case VT_FOREIGN_FUNCTION: return hash_bits((uint64_t)value.u.ptr);
	default:
		sno_unreachable;
	}
#ifdef sno_USE_ASSERT
	return sno_FALSE;
#endif
}

static sno_Bool value_union_equals(ValueUnion a, ValueUnion b, ValueType type) {
	switch (type) {
	case VT_NONE: return sno_TRUE;
	case VT_BOOL:
	case VT_NUMBER: return a.number == b.number;
	case VT_STRING:;
	case VT_ARRAY:
	case VT_TABLE:
	case VT_BYTECODE:
	case VT_FOREIGN_FUNCTION: return a.ptr == b.ptr;
	default:
		sno_unreachable;
	}
#ifdef sno_USE_ASSERT
	return sno_FALSE;
#endif
}

static sno_Bool table_find_entry(
	Table* table,
	Value key,
	TableEntry** out_entry
) {
	sno_assert_ptr(table);
	sno_assert(sno_is_power_of_2(table->capacity_mask + 1));
	sno_assert(table->capacity_mask + 1 >= TABLE_MIN_CAPACITY);
	sno_assert(table->capacity_mask + 1 <= TABLE_MAX_CAPACITY);
	sno_assert_ptr(out_entry);

	Hash hash = hash_value(key);
	uint32_t start_index = hash & table->capacity_mask;
	uint32_t index = start_index;
	TableEntry* first_tombstone = NULL;
	while (1) {
		TableEntry* entry = &table->entries[index];
		if (entry->key_type == VT_UNDEFINED) {
			if (first_tombstone) {
				*out_entry = first_tombstone;
			} else {
				*out_entry = entry;
			}
			return sno_FALSE;
		}
		if (entry->key_type != VT_TOMBSTONE) {
			if (entry->key_type == key.type) {
				if (
					value_union_equals(
						entry->key_union,
						key.u,
						entry->key_type
					)
				) {
					*out_entry = entry;
					return sno_TRUE;
				}
			}
		} else {
			if (first_tombstone == NULL) {
				first_tombstone = entry;
			}
		}
		index++;
		index &= table->capacity_mask;
		if (index == start_index) {
			break;
		}
	}
	// Table was all tombstones
	sno_assert_ptr(first_tombstone);
	*out_entry = first_tombstone;
	return sno_FALSE;
}

static sno_Bool table_insert_entry(Table* table, Value key, Value value) {
	sno_assert_ptr(table);
	sno_assert(sno_is_power_of_2(table->capacity_mask + 1));
	sno_assert(table->capacity_mask + 1 >= TABLE_MIN_CAPACITY);
	sno_assert(table->capacity_mask + 1 <= TABLE_MAX_CAPACITY);
	sno_assert(value_type_is_valid(key.type));
	sno_assert(value_type_is_valid(value.type));

	TableEntry* entry = NULL;
	if (table_find_entry(table, key, &entry)) {
		// Key already exists, overwrite value
		entry->value_type = value.type;
		entry->value_union = value.u;
		return sno_TRUE;
	} else {
		entry->key_type = key.type;
		entry->key_union = key.u;
		entry->value_type = value.type;
		entry->value_union = value.u;
		return sno_FALSE;
	}
}

static void resize_table(
	sno_State* state,
	Table* table,
	uint32_t new_capacity
) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	sno_assert_ptr(sno_is_power_of_2(new_capacity));
	sno_assert_ptr(new_capacity >= TABLE_MIN_CAPACITY);
	sno_assert(new_capacity <= TABLE_MAX_CAPACITY);
	// Create the new empty hash table.
	TableEntry* new_entries = state_alloc(
		state,
		new_capacity * sizeof(TableEntry)
	);
	for (uint32_t i = 0; i < new_capacity; i++) {
		new_entries[i].key_type = VT_UNDEFINED;
	}

	TableEntry* old_entries = table->entries;
	uint32_t old_capacity = table->capacity_mask + 1;
	uint32_t old_count = table->count;
	table->entries = new_entries;
	table->capacity_mask = new_capacity - 1;
	table->count = old_count;
	
	// Re-add the existing entries
	uint32_t count_found = 0;
	if (old_count > 0) {
		for (uint32_t i = 0; i < old_capacity; i++) {
			TableEntry* entry = &old_entries[i];
			// Don't copy empty entries or tombstones
			if (entry->key_type < 0) {
				continue;
			}
			count_found++;
			Value entry_key;
			Value entry_value;
			entry_key.type = entry->key_type;
			entry_key.u = entry->key_union;
			entry_value.type = entry->value_type;
			entry_value.u = entry->value_union;
			table_insert_entry(table, entry_key, entry_value);
		}
	}
	sno_assert(old_count == count_found);
	sno_assert(old_count == table->count);
	if (old_entries) {
		state_free(
			state,
			(old_capacity + 1) * sizeof(TableEntry),
			old_entries
		);
	}
}

sno_Bool table_set_key(
	sno_State* state,
	Table* table,
	Value key,
	Value value
) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	if (
		table->count + 1 >
		(table->capacity_mask + 1) * TABLE_RESIZE_LOAD_PERCENT / 100
	) {
		uint32_t new_capacity = sno_max(
			(table->capacity_mask + 1) *
			TABLE_GROWTH_FACTOR,
			TABLE_MIN_CAPACITY
		);
		if (new_capacity > TABLE_MAX_CAPACITY) {
			state_throw(
				state,
				EXCEPTION_RUNTIME_ERROR,
				sno_string_comma_length("Table too big")
			);
		}
		resize_table(state, table, new_capacity);
	}
	if (!table_insert_entry(table, key, value)) {
		table->count++;
		return sno_FALSE;
	}
	return sno_TRUE;
}

sno_Bool table_get_key(
	sno_State* state,
	Table* table,
	Value key,
	Value* out_value
) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	sno_assert_ptr(out_value);
	if (table->count == 0) {
		return sno_FALSE;
	}
	TableEntry* entry = NULL;
	if (table_find_entry(table, key, &entry)) {
		(*out_value).type = entry->value_type;
		(*out_value).u = entry->value_union;
		return sno_TRUE;
	}
	return sno_FALSE;
}

sno_Bool table_remove_key(
	sno_State* state,
	Table* table,
	Value key
) {
	sno_assert_ptr(state);
	sno_assert_ptr(table);
	if (table->count == 0) {
		return sno_FALSE;
	}
	TableEntry* entry = NULL;
	if (table_find_entry(table, key, &entry)) {
		entry->key_type = VT_TOMBSTONE;
		table->count--;
	} else {
		return sno_FALSE;
	}
	if (
		table->capacity_mask + 1 >= TABLE_MIN_CAPACITY &&
		table->count < (table->capacity_mask + 1) / TABLE_GROWTH_FACTOR *
		TABLE_RESIZE_LOAD_PERCENT / 100
	) {
		uint32_t new_capacity = sno_max(
			(table->capacity_mask + 1) /
			TABLE_GROWTH_FACTOR,
			TABLE_MIN_CAPACITY
		);
		resize_table(state, table, new_capacity);
	}
	return sno_TRUE;
}
