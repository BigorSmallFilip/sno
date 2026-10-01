#include "sno_string.h"

#include "sno_state.h"

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
	string_table->strings = state_alloc(state, sizeof(IString*) * capacity);
}

void resize_string_interning_table(sno_GlobalState* state, size_t new_capacity) {
	sno_assert_ptr(state);
	sno_assert_ptr(state->string_table.strings);
	sno_assert(new_capacity >= 8);
	sno_assert(sno_is_power_of_2(new_capacity));

	StringInterningTable* string_table = &state->string_table;
	IString** new_array = state_alloc(state, sizeof(IString*) * new_capacity);
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
