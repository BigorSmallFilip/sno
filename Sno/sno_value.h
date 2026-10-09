#ifndef sno_VALUE_H
#define sno_VALUE_H

#include "sno.h"
#include "sno_mem.h"
#include "sno_state.h"

typedef enum ValueType {
	VT_NONE,
	VT_BOOL,
	VT_NUMBER,
	VT_LINALG,
	VT_STRING,
	VT_ARRAY,
	VT_TABLE,
	VT_FUNCTION,
	NUM_VALUE_TYPES,
} ValueType;

typedef uint32_t Hash;

typedef union ValueUnion {
	uint64_t i;
	void* ptr;
	sno_Number number;
	struct IString* string;
	struct Array* arr;
} ValueUnion;

typedef struct Value {
	ValueType type;
	ValueUnion u;
} Value;

//DECLARE_GENERIC_DYN_ARRAY(Value, Value, value);

typedef struct Array {
	gc_object_header;
	//ValueDynArray values;
} Array;

#endif
