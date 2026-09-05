#ifndef sno_STRING_H
#define sno_STRING_H

#include "sno_utility.h"

typedef struct sno_String {
	struct sno_String* next;
	uint8_t gc_mark;
	sno_Hash hash;
	size_t length;
} sno_String;

#define sno_string_chars(str) ((str) + 1)

typedef struct sno_StringInterningTable {
	sno_String** strings;
	uint32_t num_strings;
	uint32_t capacity_mask; // Capacity - 1 since it is mainly used like a bitmask
} sno_StringInterningTable;

void sno_init_string_interning_table(struct sno_State* state, uint32_t capacity);

#endif
