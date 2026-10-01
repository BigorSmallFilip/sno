#include "sno_string.h"

#include "sno_state.h"
#include <string.h>
#include <stdio.h>

static Hash hash_string(const char* string, size_t length) {
	sno_assert_ptr(string);
	// I stole Lua's string hashing algorithm
	Hash hash = (uint32_t)length;
	// If string is too long, don't hash all its chars
	uint32_t step = ((uint32_t)length >> 5) + 1;
	for (uint32_t l1 = (uint32_t)length; l1 >= step; l1 -= step) {
		hash = hash ^ ((hash << 5) + (hash >> 2) + (char)string[l1 - 1]);
	}
	return hash;
}

void init_string_interning_table(sno_GlobalState* state, size_t capacity) {
	sno_assert_ptr(state);
	sno_assert(capacity >= 8);
	sno_assert(sno_is_power_of_2(capacity));

	StringInterningTable* string_table = &state->string_table;
	string_table->capacity_mask = capacity - 1;
	string_table->num_strings = 0;
	string_table->strings = (IString**)state_alloc(state, sizeof(IString*) * capacity);
	memset(string_table->strings, 0, sizeof(IString*) * capacity);
}

void resize_string_interning_table(sno_GlobalState* state, size_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert(new_capacity >= 8);
	sno_assert(sno_is_power_of_2(new_capacity));

	StringInterningTable* string_table = &state->string_table;
	IString** new_array = (IString**)state_alloc(state, sizeof(IString*) * new_capacity);
	memset(new_array, 0, sizeof(IString*) * new_capacity);
	size_t new_capacity_mask = new_capacity - 1;
	size_t iter_count = 0;
	for (size_t i = 0; i < string_table->capacity_mask + 1; i++) {
		IString* str = string_table->strings[i];
		while (str != NULL) {
			IString* next = str->next;
			if (!new_array[str->hash & new_capacity_mask]) {
				str->next = NULL;
				new_array[str->hash & new_capacity_mask] = str;
			} else {
				// Insert at front
				IString* old_first = new_array[str->hash & new_capacity_mask];
				str->next = old_first;
				new_array[str->hash & new_capacity_mask] = str;
			}
			str = next;
			iter_count++;
		}
	}
	sno_assert(iter_count == string_table->num_strings);
	state_free(
		state,
		(string_table->capacity_mask + 1) * sizeof(IString*),
		string_table->strings
	);
	string_table->strings = new_array;
	string_table->capacity_mask = new_capacity_mask;
}

void free_string_interning_table(sno_GlobalState* state) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);

	StringInterningTable* string_table = &state->string_table;
	for (size_t i = 0; i < string_table->capacity_mask + 1; i++) {
		IString* iter = string_table->strings[i];
		while (iter != NULL) {
			IString* next = iter->next;
			state_free(state, sizeof(IString) + iter->length, iter);
			iter = next;
		};
	}
	state_free(
		state,
		(string_table->capacity_mask + 1) * sizeof(IString*),
		string_table->strings
	);
}

void print_string_interning_table(const sno_GlobalState* state) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);

	const StringInterningTable* string_table = &state->string_table;
	size_t num_filled_buckets = 0;
	size_t num_lone_strings = 0;
	for (size_t i = 0; i < string_table->capacity_mask + 1; i++) {
		IString* str_iter = string_table->strings[i];
		if (str_iter) {
			num_filled_buckets++;
			if (str_iter->next == NULL) {
				num_lone_strings++;
			}
		}
		while (str_iter != NULL) {
			printf(
				"{ modhash = %6u, hash = %08X, len = %3u, \"%.*s\"",
				(unsigned int)i,
				(unsigned int)str_iter->hash,
				(unsigned int)str_iter->length,
				(unsigned int)str_iter->length,
				istring_chars(str_iter)
			);
			printf(" }\n");
			str_iter = str_iter->next;
		}
	}
	printf("Num strings = %u\n", (unsigned int)string_table->num_strings);
	printf("Num buckets = %u\n", (unsigned int)(string_table->capacity_mask + 1));
	printf("Num used buckets = %u\n", (unsigned int)num_filled_buckets);
	printf(
		"Bucket usage = %g%%\n",
		100.0 * (double)num_filled_buckets / (double)(string_table->capacity_mask + 1)
	);
	printf(
		"Ratio of strings to buckets = %g%%\n",
		100.0 * (double)string_table->num_strings / (double)(string_table->capacity_mask + 1)
	);
	printf(
		"%% of strings in buckets alone = %g%%\n",
		100.0 * (double)num_lone_strings / (double)string_table->num_strings
	);
	printf(
		"%% of strings in top buckets = %g%%\n",
		100.0 * (double)num_filled_buckets / (double)string_table->num_strings
	);
}



static void set_string_swizzling(IString* str) {
	str->swizzles = 0;
	str->swizzle_max = 0;
	str->swizzle_repeats = 0;
	if (str->length < 1 || str->length > 4) {
		// Not right size for valid swizzle
		return;
	}
	int swizzle_type = 0; // xyzw: 0, rgba: 1
	{
		// First swizzle char
		unsigned char c = istring_chars(str)[0];
		switch (c) {
		case 'x': { str->swizzles = 0; str->swizzle_max = 1; break; }
		case 'y': { str->swizzles = 1; str->swizzle_max = 2; break; }
		case 'z': { str->swizzles = 2; str->swizzle_max = 3; break; }
		case 'w': { str->swizzles = 3; str->swizzle_max = 4; break; }
		case 'r': { str->swizzles = 0; str->swizzle_max = 1; swizzle_type = 1; break; }
		case 'g': { str->swizzles = 1; str->swizzle_max = 2; swizzle_type = 1; break; }
		case 'b': { str->swizzles = 2; str->swizzle_max = 3; swizzle_type = 1; break; }
		case 'a': { str->swizzles = 3; str->swizzle_max = 4; swizzle_type = 1; break; }
		default: { return; }
		}
	}

	if (str->length == 1) { return; }

	// Next chars
	switch (swizzle_type) {
	case 0: { // xyzw
		for (size_t i = 1; i < str->length; i++) {
			unsigned char c = istring_chars(str)[i];
			switch (c) {
			case 'x': { str->swizzles |= 0 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 1); break; }
			case 'y': { str->swizzles |= 1 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 2); break; }
			case 'z': { str->swizzles |= 2 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 3); break; }
			case 'w': { str->swizzles |= 3 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 4); break; }
			default: { str->swizzle_max = 0; return; }
			}
		}
	} break;
	case 1: { // rgba
		for (size_t i = 1; i < str->length; i++) {
			unsigned char c = istring_chars(str)[i];
			switch (c) {
			case 'r': { str->swizzles |= 0 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 1); break; }
			case 'g': { str->swizzles |= 1 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 2); break; }
			case 'b': { str->swizzles |= 2 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 3); break; }
			case 'a': { str->swizzles |= 3 << (i * 2); str->swizzle_max = sno_max(str->swizzle_max, 4); break; }
			default: { str->swizzle_max = 0; return; }
			}
		}
	} break;
	}
}

static IString* create_new_interned_string(
	sno_GlobalState* state,
	const char* string,
	size_t length,
	Hash hash,
	IString* iter
) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert_ptr(string);

	StringInterningTable* string_table = &state->string_table;
	IString* string_obj = (IString*)state_alloc(state, sizeof(IString) + length);
	// Check if GC happened maybe?
	iter = state->string_table.strings[hash & state->string_table.capacity_mask];
	if (iter) {
		while (iter->next) {
			iter = (IString*)iter->next;
		}
	}

	sno_assert_ptr(string);
	string_obj->hash = hash;
	string_obj->length = length;
	string_obj->next = NULL;
	string_obj->gc_mark = 0;
	string_obj->gc_type = OT_STRING;
	memcpy((char*)istring_chars(string_obj), string, length);
	set_string_swizzling(string_obj);
	if (iter == NULL) {
		// No existing string in bucket
		sno_assert(
			string_table->strings[hash & string_table->capacity_mask] == NULL
		);
		string_table->strings[hash & string_table->capacity_mask] = string_obj;
	} else {
		// Insert at the end
		sno_assert(iter->next == NULL);
		iter->next = string_obj;
	}
	string_table->num_strings++;
	if (string_table->num_strings > string_table->capacity_mask) {
		if (string_table->capacity_mask >= 1000000) {
			
		}
		resize_string_interning_table(
			state,
			(string_table->capacity_mask + 1) << 1
		);
	}
	return string_obj;
}

const IString* create_string(
	sno_GlobalState* state,
	const char* const string,
	size_t length
) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert_ptr(string);

	StringInterningTable* string_table = &state->string_table;
	Hash hash = hash_string(string, length);
	IString* iter = string_table->strings[hash & string_table->capacity_mask];
	while (iter != NULL) {
		if (
			iter->length == length &&
			iter->hash == hash &&
			memcmp(istring_chars(iter), string, length) == 0
		) {
			return iter;
		}
		if (iter->next) {
			iter = (IString*)iter->next;
		} else {
			break;
		}
	}
	return create_new_interned_string(state, string, length, hash, iter);
}