#ifndef sno_STRING_H
#define sno_STRING_H

#include "sno_utility.h"
#include "sno_value.h"

enum {
	sno_STR_LENGTH,
	sno_STR_COUNT,
	sno_NUM_BUILTIN_STRINGS,
};
extern const char* sno_builtin_strings[];



// Buffer string
typedef struct sno_BString {
	int temp;
};



// Interned string
typedef struct sno_IString {
	sno_gc_string_header;
	uint8_t swizzle_len;
	uint8_t swizzles;
	uint8_t builtin_id;
	sno_Hash hash;
	size_t length;
} sno_IString;

#define sno_string_chars(str) ((const char*)((str) + 1))

typedef struct sno_StringInterningTable {
	sno_IString** strings;
	size_t num_strings;
	size_t capacity_mask; // Capacity - 1 since it is mainly used like a bitmask
} sno_StringInterningTable;

void sno_init_string_interning_table(struct sno_State* state, size_t capacity);
void sno_resize_string_interning_table(struct sno_State* state, size_t new_capacity);
void sno_free_string_interning_table(struct sno_State* state);
void sno_print_string_interning_table(const struct sno_State* state);

const sno_IString* sno_create_string(struct sno_State* state, const char* string, size_t length);
#define sno_create_string_from_literal(state, string) (sno_create_string(state, sno_str_comma_len(string)))

const sno_IString* sno_load_string_from_file(
	struct sno_State* state,
	const char* const path,
	size_t path_length
);

const sno_IString* sno_string_to_lowercase(
	struct sno_State* state,
	const sno_IString* string
);

const sno_IString* sno_string_to_uppercase(
	struct sno_State* state,
	const sno_IString* string
);

void sno_free_string(
	struct sno_State* state,
	sno_IString* string,
	sno_IString* prev
);

#endif
