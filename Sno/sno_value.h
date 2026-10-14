#ifndef sno_VALUE_H
#define sno_VALUE_H

#include "sno.h"
#include "sno_mem.h"

typedef enum ValueType {
	VT_NONE,
	VT_BOOL,
	VT_NUMBER,
	VT_LINALG,
	VT_STRING,
	VT_ARRAY,
	VT_TABLE,
	VT_BYTECODE,
	VT_FOREIGN_FUNCTION,
	NUM_VALUE_TYPES,
	VT_UNDEFINED = -1,
	VT_TOMBSTONE = -2,
} ValueType;

#define value_type_is_valid(type) \
	((type) >= VT_NONE && (type) < NUM_VALUE_TYPES)

typedef union ValueUnion {
	uint64_t i;
	void* ptr;
	sno_Number number;
	struct IString* string;
	struct Array* arr;
	struct Table* table;
} ValueUnion;

typedef struct Value {
	ValueType type;
	ValueUnion u;
} Value;

void print_value(Value value);

DECLARE_GENERIC_DYN_ARRAY(Value, Value, value);

typedef struct Array {
	gc_object_header;
	ValueDynArray values;
} Array;

Array* create_array(struct sno_VMState* vm);



typedef struct TableEntry {
	ValueType key_type;
	ValueType value_type;
	ValueUnion key_union;
	ValueUnion value_union;
} TableEntry;

#define TABLE_MIN_CAPACITY 8
#define TABLE_MAX_CAPACITY ((UINT32_MAX >> 1) + 1)
#define TABLE_GROWTH_FACTOR 2
#define TABLE_RESIZE_LOAD_PERCENT 75

typedef struct Table {
	gc_object_header;
	uint32_t capacity_mask; // Capacity - 1 to be used as a bit mask
	uint32_t count;
	TableEntry* entries;
} Table;

Table* create_table(struct sno_VMState* vm);
sno_Bool table_set_key(
	struct sno_VMState* vm,
	Table* table,
	Value key,
	Value value
);
sno_Bool table_get_key(
	struct sno_VMState* vm,
	Table* table,
	Value key,
	Value* out_value
);

#endif
