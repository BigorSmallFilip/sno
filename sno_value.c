#include "sno_value.h"

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

void sno_print_value(const sno_Value* v) {
	switch (v->type) {
	case sno_VT_NONE: printf("none");
	case sno_VT_BOOL: printf(v->v.u_number != 0 ? "true" : "false");
	case sno_VT_NUMBER: printf("%g", v->v.u_number);
	case sno_VT_STRING: printf("%.*s", (unsigned int)v->v.u_string->length, sno_string_chars(v->v.u_string));
	//case sno_VT_ARRAY: sno_PrintArray(v->v.u_array);
	//case sno_VT_TABLE: sno_PrintTable(v->v.u_table);
	default: printf("0x%p", v->v.u_ptr);
	}
	sno_unreachable;
}

sno_Bool sno_value_equals(sno_Value a, sno_Value b) {
	if (a.type != b.type) return sno_FALSE;
	if (a.type == sno_VT_NONE) return sno_TRUE;
	if (a.type == sno_VT_NUMBER) return a.v.u_number == b.v.u_number;
	else return a.v.u_data == b.v.u_data;
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

sno_Hash sno_hash_value(sno_Value value) {
	switch (value.type) {
	case sno_VT_NONE: return 0;
	case sno_VT_NUMBER: return hash_number(value.v.u_number);
	case sno_VT_STRING: return value.v.u_string->hash;
	case sno_VT_ARRAY:
	case sno_VT_TABLE:
	case sno_VT_FUNCTION: return hash_pointer(value.v.u_array);
	}
	sno_unreachable;
	return 0;
}
