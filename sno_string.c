#include "sno_string.h"

#include "sno_state.h"
#include "sno_mem.h"
#include <string.h>



static sno_IString* create_new_interned_string(
	sno_State* state,
	const char* string,
	size_t length,
	sno_Hash hash,
	sno_IString* iter
);

static sno_Hash hash_string(const char* string, size_t length) {
	sno_assert_ptr(string);
	// I stole Lua's string hashing algorithm
	sno_Hash hash = (uint32_t)length;
	uint32_t step = ((uint32_t)length >> 5) + 1;  // If string is too long, don't hash all its chars
	for (uint32_t l1 = (uint32_t)length; l1 >= step; l1 -= step)
		hash = hash ^ ((hash << 5) + (hash >> 2) + (char)string[l1 - 1]);
	return hash;
}



const char* sno_builtin_strings[] = {
	"length",
	"count",
};

static void init_builtin_strings(sno_State* state) {
	sno_StringInterningTable* string_table = &state->string_table;
	for (int i = 0; i < sno_NUM_BUILTIN_STRINGS; i++) {
		const char* string = sno_builtin_strings[i];
		size_t length = strlen(string);
		sno_Hash hash = hash_string(string, length);
		sno_IString* iter = string_table->strings[hash & string_table->capacity_mask];
		while (iter != NULL) {
			if (iter->next) {
				iter = iter->next;
			} else {
				break;
			}
		}
		sno_IString* string_obj = create_new_interned_string(state, string, length, hash, iter);
		string_obj->builtin_id = i;
	}
	
}



void sno_init_string_interning_table(sno_State* state, size_t capacity) {
	sno_assert_ptr(state);
	sno_assert(capacity >= 16 && capacity <= sno_SIZE_T_LIMIT);
	sno_assert(sno_is_power_of_2(capacity));

	sno_StringInterningTable* string_table = &state->string_table;
	string_table->capacity_mask = capacity - 1;
	string_table->num_strings = 0;
	string_table->strings = sno_calloc(state, capacity, sizeof(sno_IString*));
	init_builtin_strings(state);
}

void sno_resize_string_interning_table(sno_State* state, size_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert(new_capacity >= 16 && new_capacity <= sno_SIZE_T_LIMIT);
	sno_assert(sno_is_power_of_2(new_capacity));

	sno_StringInterningTable* string_table = &state->string_table;
	sno_IString** new_array = sno_calloc(state, new_capacity, sizeof(sno_IString*));
	size_t new_capacity_mask = new_capacity - 1;
	for (size_t i = 0; i < string_table->capacity_mask + 1; i++) {
		sno_IString* str = string_table->strings[i];
		while (str != NULL) {
			if (!new_array[str->hash & new_capacity_mask]) {
				str->next = NULL;
				new_array[str->hash & new_capacity_mask] = str;
			} else {
				// Insert at front
				sno_IString* old_first = new_array[str->hash & new_capacity_mask];
				str->next = old_first;
				new_array[str->hash & new_capacity_mask] = str;
			}
			str = str->next;
		}
	}
	sno_free(state, string_table->strings, (string_table->capacity_mask + 1) * sizeof(sno_IString*));
	string_table->strings = new_array;
	string_table->capacity_mask = new_capacity_mask;
}

void sno_free_string_interning_table(sno_State* state) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);

	sno_StringInterningTable* string_table = &state->string_table;
	for (size_t i = 0; i < string_table->capacity_mask + 1; i++) {
		sno_IString* iter = string_table->strings[i];
		while (iter != NULL) {
			sno_IString* next = iter->next;
			sno_free(state, iter, iter->length + sizeof(sno_IString));
			iter = next;
		};
	}
	sno_free(state, string_table->strings, (string_table->capacity_mask + 1) * sizeof(sno_IString*));
}

void sno_print_string_interning_table(const sno_State* state) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);

	const sno_StringInterningTable* string_table = &state->string_table;
	size_t num_filled_buckets = 0;
	size_t num_lone_strings = 0;
	for (size_t i = 0; i < string_table->capacity_mask + 1; i++) {
		sno_IString* str_iter = string_table->strings[i];
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
				sno_string_chars(str_iter)
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



static sno_IString* create_new_interned_string(
	sno_State* state,
	const char* string,
	size_t length,
	sno_Hash hash,
	sno_IString* iter
) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert_ptr(string);
	sno_assert(length <= sno_SIZE_T_LIMIT);

	sno_StringInterningTable* string_table = &state->string_table;
	sno_IString* string_obj = sno_malloc(state, sizeof(sno_IString) + length);
	sno_assert_ptr(string);
	string_obj->hash = hash;
	string_obj->length = length;
	string_obj->next = NULL;
	string_obj->gc_mark = 0;
	string_obj->gc_type = sno_OT_STRING;
	memcpy((char*)sno_string_chars(string_obj), string, length);
	if (iter == NULL) {
		// No existing string in bucket
		sno_assert(string_table->strings[hash & string_table->capacity_mask] == NULL);
		string_table->strings[hash & string_table->capacity_mask] = string_obj;
	} else {
		// Insert at the end
		sno_assert(iter->next == NULL);
		iter->next = string_obj;
	}
	string_table->num_strings++;
	if (string_table->num_strings > string_table->capacity_mask) {
		if (string_table->capacity_mask >= 1000000) {
			// TODO: Remove this lol
			sno_panic("Wayy too many strings?? I don't really know if this should panic lol");
		}
		sno_resize_string_interning_table(state, (string_table->capacity_mask + 1) << 1);
	}

	return string_obj;
}

const sno_IString* sno_create_string(sno_State* state, const char* string, size_t length) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert_ptr(string);
	sno_assert(length <= sno_SIZE_T_LIMIT);
	
	sno_StringInterningTable* string_table = &state->string_table;
	sno_Hash hash = hash_string(string, length);
	sno_IString* iter = string_table->strings[hash & string_table->capacity_mask];
	while (iter != NULL) {
		if (
			iter->length == length &&
			iter->hash == hash &&
			memcmp(sno_string_chars(iter), string, length) == 0
		) {
			return iter;
		}
		if (iter->next) {
			iter = iter->next;
		} else {
			break;
		}
	}
	return create_new_interned_string(state, string, length, hash, iter);
}

const sno_IString* sno_load_string_from_file(
	sno_State* state,
	const char* const path,
	size_t path_length
) {
	sno_assert_ptr(state);
	sno_assert_ptr(path);
	char path_zero[sno_STACK_BUFFER_LENGTH];
	memcpy(path_zero, path, path_length);
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
	fseek(file, 0L, SEEK_END);
	long size = ftell(file);
	sno_assert(size >= 0);
	rewind(file);
	sno_assert(size >= 0);
	char* filebuffer = (char*)sno_malloc(state, (size_t)size);
	if (!filebuffer) {
		fclose(file);
		return NULL;
	}
	size_t readsize = fread(filebuffer, sizeof(char), (size_t)size, file);
	sno_assert(readsize <= ULONG_MAX);
	if (ferror(file) != 0) {
		sno_free(state, filebuffer, (size_t)size);
		fclose(file);
		return NULL;
	}
	fclose(file);
	return sno_create_string(state, filebuffer, readsize);
}

void sno_free_string(
	sno_State* state,
	sno_IString* string,
	sno_IString* prev
) {
	if (prev) {
		prev->next = string->next;
	}
	sno_free(state, string, sizeof(sno_IString) + string->length);
}
