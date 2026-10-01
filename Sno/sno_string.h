#ifndef sno_STRING_H
#define sno_STRING_H

#include "sno.h"
#include "sno_mem.h"
#include "sno_value.h"

typedef struct IString {
	gc_object_string_header;
	uint8_t swizzle_max : 3;
	uint8_t siwzzle_repeats : 1;
	uint8_t swizzles;
	Hash hash;
	size_t length;
} IString;

#define istring_chars(istring) ((const char* const)(istring + 1))

typedef struct StringInterningTable {
	IString** strings;
	size_t num_strings;
	size_t capacity_mask; // Capacity - 1 since it is mainly used like a bitmask
} StringInterningTable;

void init_string_interning_table(struct sno_GlobalState* state, size_t capacity);
void resize_string_interning_table(struct sno_GlobalState* state, size_t new_capacity);
void free_string_interning_table(struct sno_GlobalState* state);
void print_string_interning_table(const struct sno_GlobalState* state);

#endif
