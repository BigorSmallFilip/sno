#ifndef sno_VALUE_H
#define sno_VALUE_H

#include "sno.h"

enum {
	VT_NONE,
	VT_BOOL,
	VT_NUMBER,
	VT_LINALG,
	VT_STRING,
	VT_ARRAY,
	VT_TABLE,
	VT_FUNCTION,
	NUM_VALUE_TYPES,
};
typedef uint8_t ValueType;

typedef uint32_t Hash;

#endif
