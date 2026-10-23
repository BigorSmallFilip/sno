#ifndef sno_STRING_H
#define sno_STRING_H

#include "sno.h"
#include "sno_mem.h"

// Non null terminated string
typedef struct String {
	char* string;
	size_t length;
} String;

// Interned string
typedef struct IString {
	gc_object_string_header;
	uint8_t swizzle_max : 3;
	uint8_t swizzle_repeats : 1;
	uint8_t swizzles;
	Hash hash;
	size_t length;
} IString;

#define istring_chars(istring) ((const char* const)((istring) + 1))

DECLARE_GENERIC_DYN_ARRAY(IString*, IString, istring);

#define STRING_SET_TOMBSTONE ((uintptr_t)1)
#define STRING_SET_MAX_CAPACITY ((UINT32_MAX >> 1) + 1)

typedef struct StringInterningSet {
	IString** strings;
	uint32_t num_strings;
	uint32_t capacity_mask; // Capacity - 1 since it is mainly used like a bitmask
} StringInterningSet;

void init_string_interning_set(struct sno_State* state, uint32_t capacity);
void resize_string_interning_set(struct sno_State* state, uint32_t new_capacity);
void free_string_interning_set(struct sno_State* state);
void print_string_interning_set(const struct sno_State* state);

IString* create_istring(
	sno_State* state,
	const char* const string,
	size_t length
);

IString* load_istring_from_file(
	struct sno_State* state,
	const char* const path,
	size_t path_length
);

#endif
