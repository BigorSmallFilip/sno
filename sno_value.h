#ifndef sno_VALUE_H
#define sno_VALUE_H

#include "sno_utility.h"
#include "sno_mem.h"

enum {
	sno_VT_NONE,
	sno_VT_BOOL,
	sno_VT_NUMBER,
	sno_VT_STRING,
	sno_VT_ARRAY,
	sno_VT_TABLE,
	sno_VT_FUNCTION,
};
typedef uint8_t sno_ValueType;

#define sno_type_is_gc(type) ((type) == sno_VT_ARRAY || (type) == sno_VT_TABLE)

extern const char* const sno_type_strings[7];
extern const char* const sno_type_strings_noun[7];

#define sno_NUMBER_FALSE ((sno_Number)0)
#define sno_NUMBER_TRUE ((sno_Number)1)

typedef union sno_ValueUnion {
	uint64_t u_data;
	void* u_ptr;
	struct sno_Value* u_stack_ptr;
	sno_Number u_number;
	struct sno_GCObject* gc_obj;
	const struct sno_IString* u_string;
	struct sno_Array* u_array;
	struct sno_Table* u_table;
	struct sno_Function* u_function;
} sno_ValueUnion;

typedef struct sno_Value {
	sno_ValueType type;
	sno_ValueUnion v;
} sno_Value;

#define sno_set_none(value)               (value).type = sno_VT_NONE;     (value).v.u_data     = 0
#define sno_set_bool(value, b)            (value).type = sno_VT_BOOL;     (value).v.u_number   = ((b) ? sno_NUMBER_TRUE : sno_NUMBER_FALSE) 
#define sno_set_false(value)              (value).type = sno_VT_BOOL;     (value).v.u_number   = sno_NUMBER_FALSE
#define sno_set_true(value)               (value).type = sno_VT_BOOL;     (value).v.u_number   = sno_NUMBER_TRUE
#define sno_set_number(value, number)     (value).type = sno_VT_NUMBER;   (value).v.u_number   = ((sno_Number)(number))
#define sno_set_string(value, string)     (value).type = sno_VT_STRING;   (value).v.u_string   = (string)
#define sno_set_array(value, arr)         (value).type = sno_VT_ARRAY;    (value).v.u_array    = (arr)
#define sno_set_table(value, table)       (value).type = sno_VT_TABLE;    (value).v.u_table    = (table)
#define sno_set_function(value, function) (value).type = sno_VT_FUNCTION; (value).v.u_function = (function)



enum {
	sno_OT_STRING,
	sno_OT_ARRAY,
	sno_OT_TABLE,
	sno_OT_FUNCTION,
	sno_OT_BYTECODE,
};
typedef uint8_t sno_GCObjectType;

typedef struct sno_GCObject {
	struct sno_GCObject* gc_next;
	uint8_t gc_mark;
	sno_GCObjectType gc_type;
	// Since this will have padding bytes, this may cause undefined behaviour in the future
	// Some padding to fix this?
	uint8_t _padding[6];
} sno_GCObject;
#define sno_gc_header struct sno_GCObject* gc_next; uint8_t gc_mark; sno_GCObjectType gc_type
#define sno_gc_string_header struct sno_GCObject* next; uint8_t gc_mark; sno_GCObjectType gc_type



typedef struct sno_Array {
	sno_gc_header;
	sno_DynArray items;
} sno_Array;

sno_Array* sno_create_array(struct sno_State* state, size_t capacity);
void sno_concat_array(struct sno_State* state, sno_Array* arr, sno_Value* items, size_t count);



typedef struct sno_Table_Node {
	sno_Bool exists;
	sno_ValueType key_type;
	sno_ValueType value_type;
	sno_ValueUnion key_union;
	sno_ValueUnion value_union;
	struct sno_Table_Node* next;
} sno_TableNode;

typedef struct sno_Table {
	sno_gc_header;
	size_t count;
	size_t capacity_mask; // Capcity - 1 since it is most often used as a bitmask
	sno_TableNode* nodes;
	struct sno_Table* prototype;
} sno_Table;

sno_Table* sno_create_table(struct sno_State* state, size_t capacity);
sno_Bool sno_table_set_or_add_key(struct sno_State* state, sno_Table* table, const sno_Value* key, const sno_Value* value);
sno_Bool sno_table_set(sno_Table* table, const sno_Value* key, const sno_Value* value);
sno_Bool sno_table_get(sno_Table* table, const sno_Value* key, sno_Value* out_value);
sno_Bool sno_table_has(sno_Table* table, const sno_Value* key);
sno_Bool sno_table_remove(struct sno_State* state, sno_Table* table, const sno_Value* key);

void sno_table_iter(sno_Table* table, size_t* bucket, sno_TableNode** node);
void sno_table_next(sno_Table* table, size_t* bucket, sno_TableNode** node);



typedef uint8_t(sno_CFunction)(struct sno_State*, uint8_t);

typedef struct sno_Function {
	sno_gc_header;
	sno_Bool is_c_function;
	//uint8_t numupvalues;
	union {
		struct sno_Bytecode* bytecode;
		sno_CFunction* c_function;
	} u;
	//sno_Value upvalues[];
} sno_Function;



void sno_free_gc_object(
	struct sno_State* state,
	sno_GCObject* obj,
	sno_GCObject* prev
);

void sno_print_value(struct sno_State* state, const sno_Value* v);

void sno_value_to_string(
	struct sno_State* state,
	sno_DynArray* string,
	const sno_Value* v,
	size_t recursion_limit
);
const struct sno_IString* sno_interpolate_string(
	struct sno_State* state,
	const sno_Value* values,
	uint32_t num_values
);

sno_Bool sno_value_equals(const sno_Value* a, const sno_Value* b);
sno_Bool sno_value_to_bool(const sno_Value* v);
sno_Hash sno_hash_value(const sno_Value* value);

sno_Function* sno_create_function(struct sno_State* state, const struct sno_Bytecode* bytecode);

#endif
