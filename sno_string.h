#ifndef sno_STRING_H
#define sno_STRING_H

#include "sno_utility.h"

typedef struct sno_String {
	struct sno_String* next;
	uint8_t gc_mark;
	uint8_t swizzle_len;
	uint8_t swizzles;
	uint8_t built_in_id;
	sno_Hash hash;
	size_t length;
} sno_String;

#define sno_string_chars(str) ((const char*)((str) + 1))

typedef struct sno_StringInterningTable {
	sno_String** strings;
	size_t num_strings;
	size_t capacity_mask; // Capacity - 1 since it is mainly used like a bitmask
} sno_StringInterningTable;

void sno_init_string_interning_table(struct sno_State* state, size_t capacity);
void sno_resize_string_interning_table(struct sno_State* state, size_t new_capacity);
void sno_free_string_interning_table(struct sno_State* state);
void sno_print_string_interning_table(const struct sno_State* state);

const sno_String* sno_create_string(struct sno_State* state, const char* string, size_t length);
#define sno_create_string_from_literal(state, string) (sno_create_string(state, sno_str_comma_len(string)))

const sno_String* sno_load_string_from_file(
	struct sno_State* state,
	const char* const path,
	size_t path_length
);

#endif
