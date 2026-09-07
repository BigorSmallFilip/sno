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

extern const char* const sno_type_strings[7];
extern const char* const sno_type_strings_noun[7];

#define sno_NUMBER_FALSE ((sno_Number)0)
#define sno_NUMBER_TRUE ((sno_Number)1)

typedef union sno_ValueUnion {
	uint64_t u_data;
	void* u_ptr;
	struct sno_Value* u_stack_ptr;
	sno_Number u_number;
	const struct sno_String* u_string;
	struct sno_Array* u_array;
	struct sno_Table* u_table;
	struct sno_Function* u_function;
} sno_ValueUnion;

typedef struct sno_Value {
	sno_ValueType type;
	sno_ValueUnion v;
} sno_Value;

typedef struct sno_GCValue {
	struct sno_GCValue* gc_next;
	uint8_t gc_mark;
	// Since this will have padding bytes, this may cause undefined behaviour in the future
	// Some padding to fix this?
	uint8_t _padding[7];
} sno_GCValue;
#define sno_gc_header struct sno_GCValue* gc_next; uint8_t gc_mark



typedef struct sno_Array {
	sno_gc_header;
	sno_DynArray items;
} sno_Array;



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
	uint32_t num_nodes;
	uint32_t capacity_mask; // Capcity - 1 since it is most often used as a bitmask
	sno_TableNode* nodes;
	struct sno_Table* prototype;
} sno_Table;



typedef void(sno_CFunction)(struct sno_State*, int);

typedef struct sno_Function {
	sno_gc_header;
	sno_Bool is_c_function;
	//uint8_t numupvalues;
	union {
		const struct sno_Bytecode* bytecode;
		sno_CFunction* c_function;
	} u;
	//sno_Value upvalues[];
} sno_Function;



void sno_print_value(const sno_Value* v);
sno_Bool sno_value_equals(sno_Value a, sno_Value b);
sno_Bool sno_value_to_bool(const sno_Value* v);
sno_Hash sno_hash_value(sno_Value value);

sno_Function* sno_create_function(struct sno_State* state, const struct sno_Bytecode* bytecode);

#endif
