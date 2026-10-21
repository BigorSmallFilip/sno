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

#define NUMBER_FALSE ((sno_Number)0)
#define NUMBER_TRUE ((sno_Number)1)

typedef union ValueUnion {
	uint64_t i;
	void* ptr;
	sno_Number number;
	struct IString* string;
	struct Array* arr;
	struct Table* table;
	struct Bytecode* bytecode;
	sno_ForeignFunction* foreign_function;
} ValueUnion;

typedef struct Value {
	ValueType type;
	ValueUnion u;
} Value;

#define set_none(v) (v).type = VT_NONE
#define set_false(v) (v).type = VT_BOOL; (v).u.number = NUMBER_FALSE
#define set_true(v) (v).type = VT_BOOL; (v).u.number = NUMBER_TRUE
#define set_bool(v, b) \
	(v).type = VT_BOOL; (v).u.number = ((b) ? NUMBER_TRUE : NUMBER_FALSE)
#define set_number(v, n) (v).type = VT_NUMBER; (v).u.number = (n)
#define set_string(v, s) (v).type = VT_STRING; (v).u.string = (s)
#define set_bytecode(v, b) (v).type = VT_BYTECODE; (v).u.bytecode = b

const char* const get_value_type_name(ValueType type);
const char* const get_value_type_noun_phrase(ValueType type);
sno_Bool value_eq(Value a, Value b);
sno_Bool value_to_bool(Value value);

void print_value(Value value);

DECLARE_GENERIC_DYN_ARRAY(Value, Value, value);

typedef struct Array {
	gc_object_header;
	ValueDynArray values;
} Array;

Array* create_array(struct sno_State* state);



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

Table* create_table(struct sno_State* state);
sno_Bool table_set_key(
	struct sno_State* state,
	Table* table,
	Value key,
	Value value
);
sno_Bool table_set_or_add_key(
	struct sno_State* state,
	Table* table,
	Value key,
	Value value
);
sno_Bool table_get_key(
	struct sno_State* state,
	Table* table,
	Value key,
	Value* out_value
);
sno_Bool table_remove_key(
	struct sno_State* state,
	Table* table,
	Value key
);

#endif
