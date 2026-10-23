#include "sno_string.h"

#include "sno_state.h"
#include <string.h>
#include <stdio.h>

DEFINE_GENERIC_DYN_ARRAY(IString*, IString, istring);

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

void init_string_interning_set(sno_State* state, uint32_t capacity) {
	sno_assert_ptr(state);
	sno_assert(sno_is_power_of_2(capacity));
	sno_assert(capacity >= STRING_SET_MIN_CAPACITY);
	sno_assert(capacity <= STRING_SET_MAX_CAPACITY);

	StringInterningSet* string_set = &state->string_set;
	string_set->capacity_mask = capacity - 1;
	string_set->num_strings = 0;
	string_set->strings = (IString**)state_alloc(state, sizeof(IString*) * capacity);
	memset(string_set->strings, 0, sizeof(IString*) * capacity);
}



static IString** find_string_set_entry(
	StringInterningSet* string_set,
	Hash hash,
	const char* string,
	size_t length
) {
	sno_assert_ptr(string_set);
	sno_assert_ptr(string);
	uint32_t start_index = hash & string_set->capacity_mask;
	uint32_t index = start_index;
	IString** first_tombstone = NULL;
	while (1) {
		IString** set_ptr = &string_set->strings[index];
		if (*set_ptr == NULL) {
			if (first_tombstone) {
				return first_tombstone;
			} else {
				return set_ptr;
			}
		} else if ((uintptr_t)(*set_ptr) == STRING_SET_TOMBSTONE) {
			if (!first_tombstone) {
				first_tombstone = set_ptr;
			}
		} else {
			IString* check_string = *set_ptr;
			sno_assert_ptr(check_string);
			if (
				check_string->length == length &&
				check_string->hash == hash &&
				memcmp(istring_chars(check_string), string, length) == 0
			) {
				return set_ptr;
			}
		}
		index++;
		index &= string_set->capacity_mask;
		if (index == start_index) {
			break;
		}
	}
	// Table was all tombstones
	sno_assert_ptr(first_tombstone);
	return first_tombstone;
}

void resize_string_interning_set(sno_State* state, uint32_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_set.strings);
	sno_assert(sno_is_power_of_2(new_capacity));
	sno_assert(new_capacity >= STRING_SET_MIN_CAPACITY);
	sno_assert(new_capacity <= STRING_SET_MAX_CAPACITY);

	StringInterningSet* string_set = &state->string_set;
	IString** new_array = (IString**)state_alloc(
		state,
		sizeof(IString*) * new_capacity
	);
	memset(new_array, 0, sizeof(IString*) * new_capacity);
	uint32_t new_capacity_mask = new_capacity - 1;
	uint32_t iter_count = 0;
	for (uint32_t i = 0; i < string_set->capacity_mask + 1; i++) {
		IString* str = string_set->strings[i];
		if (str != NULL && (uintptr_t)str != STRING_SET_TOMBSTONE) {
			uint32_t new_i = str->hash & new_capacity_mask;
			while (1) {
				if (new_array[new_i] == NULL) {
					new_array[new_i] = str;
					break;
				}
				new_i++;
				new_i &= new_capacity_mask;
				sno_assert(new_i != (str->hash & new_capacity_mask));
			}
			iter_count++;
		}
	}
	sno_assert(iter_count == string_set->num_strings);
	state_free(
		state,
		(string_set->capacity_mask + 1) * sizeof(IString*),
		string_set->strings
	);
	string_set->strings = new_array;
	string_set->capacity_mask = new_capacity_mask;
}

void free_string_interning_set(sno_State* state) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_set.strings);

	StringInterningSet* string_set = &state->string_set;
	for (uint32_t i = 0; i < string_set->capacity_mask + 1; i++) {
		IString* string = string_set->strings[i];
		while (string != NULL && (uintptr_t)string != STRING_SET_TOMBSTONE) {
			state_free(state, sizeof(IString) + string->length, string);
		};
	}
	state_free(
		state,
		(string_set->capacity_mask + 1) * sizeof(IString*),
		string_set->strings
	);
}

void print_string_interning_set(const sno_State* state) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_set.strings);

	const StringInterningSet* string_set = &state->string_set;
	uint32_t num_filled_buckets = 0;
	uint32_t num_lone_strings = 0;
	for (uint32_t i = 0; i < string_set->capacity_mask + 1; i++) {
		IString* str = string_set->strings[i];
		if (str != NULL && (uintptr_t)str != STRING_SET_TOMBSTONE) {
			printf(
				"{ modhash = %6u, hash = %08X, len = %3u, \"%.*s\"",
				(unsigned int)i,
				(unsigned int)str->hash,
				(unsigned int)str->length,
				(unsigned int)str->length,
				istring_chars(str)
			);
			printf(" }\n");
		}
	}
	printf("Num strings = %u\n", (unsigned int)string_set->num_strings);
	printf("Num buckets = %u\n", (unsigned int)(string_set->capacity_mask + 1));
	printf("Num used buckets = %u\n", (unsigned int)num_filled_buckets);
	printf(
		"Bucket usage = %g%%\n",
		100.0 * (double)num_filled_buckets / (double)(string_set->capacity_mask + 1)
	);
	printf(
		"Ratio of strings to buckets = %g%%\n",
		100.0 * (double)string_set->num_strings / (double)(string_set->capacity_mask + 1)
	);
	printf(
		"%% of strings in buckets alone = %g%%\n",
		100.0 * (double)num_lone_strings / (double)string_set->num_strings
	);
	printf(
		"%% of strings in top buckets = %g%%\n",
		100.0 * (double)num_filled_buckets / (double)string_set->num_strings
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

static IString* create_new_istring(
	sno_State* state,
	const char* string,
	size_t length,
	Hash hash,
	IString** set_ptr
) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_set.strings);
	sno_assert_ptr(string);

	StringInterningSet* string_set = &state->string_set;
	IString* string_obj = (IString*)state_alloc(state, sizeof(IString) + length);
	// TODO: If a GC happens during this alloc, then the set_ptr argument
	// will be invalidated and needs to be recalculated
	
	sno_assert_ptr(string);
	string_obj->hash = hash;
	string_obj->length = length;
	string_obj->gc_next = NULL;
	string_obj->gc_mark = 0;
	string_obj->gc_type = OT_STRING;
	memcpy((char*)istring_chars(string_obj), string, length);
	set_string_swizzling(string_obj);
	
	*set_ptr = string_obj;
	string_set->num_strings++;
	if (
		(size_t)string_set->num_strings + 1 >
		(size_t)(string_set->capacity_mask + 1) *
			STRING_SET_RESIZE_LOAD_PERCENT / 100
	) {
		resize_string_interning_set(
			state,
			(string_set->capacity_mask + 1) << 1
		);
	}
	return string_obj;
}

IString* create_istring(
	sno_State* state,
	const char* const string,
	size_t length
) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_set.strings);
	sno_assert_ptr(string);

	StringInterningSet* string_set = &state->string_set;
	Hash hash = hash_string(string, length);
	IString** set_ptr = find_string_set_entry(
		string_set,
		hash,
		string,
		length
	);
	sno_assert_ptr(set_ptr);
	if (*set_ptr != NULL && (uintptr_t)(*set_ptr) != STRING_SET_TOMBSTONE) {
		return *set_ptr;
	} else {
		return create_new_istring(state, string, length, hash, set_ptr);
	}
}

IString* load_istring_from_file(
	sno_State* state,
	const char* const path,
	size_t path_length
) {
	sno_assert_ptr(state);
	sno_assert_ptr(path);
	char path_zero[2048];
	(void)memcpy(path_zero, path, path_length);
	path_zero[path_length] = '\0';
	FILE* file = fopen(path_zero, "r");
	if (!file) {
		printf(
			sno_ANSI_RED "Couldn't open file \"%.*s\"\n" sno_ANSI_NORMAL,
			(unsigned int)path_length,
			path
		);
		return NULL;
	}
	(void)fseek(file, 0L, SEEK_END);
	long size = ftell(file);
	sno_assert(size >= 0);
	rewind(file);
	sno_assert(size >= 0);
	char* filebuffer = (char*)state_alloc(state, (size_t)size);
	if (!filebuffer) {
		(void)fclose(file);
		return NULL;
	}
	size_t readsize = fread(filebuffer, sizeof(char), (size_t)size, file);
	if (ferror(file) != 0) {
		state_free(state, (size_t)size, filebuffer);
		(void)fclose(file);
		return NULL;
	}
	(void)fclose(file);
	return create_istring(state, filebuffer, readsize);
}
