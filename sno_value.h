#ifndef sno_VALUE_H
#define sno_VALUE_H

#include "sno_utility.h"
#include "sno_string.h"

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

#endif
