#ifndef sno_LIB_H
#define sno_LIB_H

#include "sno_utility.h"
#include "sno_value.h"

typedef struct sno_Library {
	const char* const name;
	size_t name_length;
	sno_CFunction* function;
} sno_Library;

#define sno_LibFn(lib_name, fn_name) { \
	sno_str_comma_len(#fn_name), \
	snol_##lib_name##_##fn_name }
#define sno_LibEnd { 0 }

extern const sno_Library sno_lib_core[];
extern const sno_Library sno_lib_math[];

void sno_load_lib_into_global_scope(struct sno_State* state, const sno_Library* lib);
sno_Table* sno_load_lib_into_table(struct sno_State* state, const sno_Library* lib);

#endif
