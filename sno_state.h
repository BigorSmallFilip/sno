#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno_utility.h"
#include "sno_string.h"
#include "sno_value.h"
#include <setjmp.h>

typedef struct sno_ExceptionJump {
	struct sno_ExceptionJump* prev;
	jmp_buf buf;
} sno_ExceptionJump;

typedef struct sno_CallInfo {
	uint32_t base;
	sno_Instruction* saved_pc;
	uint8_t num_args;
	uint8_t num_returns;
} sno_CallInfo;

#define sno_MAX_STACK 256
#define sno_stack_base(state) ((state)->stack + (state)->stack_base)

typedef struct sno_State {
	sno_Value* stack;
	uint32_t stack_base;
	uint32_t stack_top;
	uint32_t stack_capacity;
	sno_DynArray call_infos;
	sno_StringInterningTable string_table;
	const sno_String* exception_msg;
	sno_ExceptionJump* exception_jump;
	sno_Table* globals;
	sno_Table* string_prototype;
	sno_Table* array_prototype;
	sno_Table* table_prototype;

	sno_GCObject* gc_list_start;
	size_t num_gc_objects;
	size_t memory_allocated;
	size_t num_allocations;
} sno_State;

sno_API sno_State* sno_create_state();
sno_API void sno_free_state(sno_State* state);

sno_API sno_Value* sno_reserve_stack(sno_State* state, uint32_t slots);

sno_API void sno_check_table(sno_State* state, uint32_t slot);

#define sno_self -1
sno_API const sno_Value* sno_get_arg(sno_State* state, int arg);
sno_API sno_Bool sno_get_bool_arg(sno_State* state, int arg);
sno_API sno_Number sno_get_number_arg(sno_State* state, int arg);
sno_API const sno_String* sno_get_string_arg(sno_State* state, int arg);
sno_API sno_Array* sno_get_array_arg(sno_State* state, int arg);
sno_API sno_Table* sno_get_table_arg(sno_State* state, int arg);
sno_API sno_Function* sno_get_function_arg(sno_State* state, int arg);

#define sno_arg(i) (state->stack[state->stack_base + 2 + (i)])
#define sno_ret(i) (state->stack[state->stack_base + (i)])

sno_API void sno_create_new_global(sno_State* state, const sno_String* name, const sno_Value* value);
sno_API void sno_set_global(sno_State* state, const sno_String* name, const sno_Value* value);
sno_API void sno_get_global(sno_State* state, const sno_String* name, const sno_Value* out_value);

sno_API void sno_print_globals(const sno_State* state);
sno_API void sno_print_exception_msg(const sno_State* state);

sno_API sno_Bool sno_try_compile_source_code(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
);

sno_API void sno_call(sno_State* state, uint8_t num_args, uint8_t num_returns);
sno_API sno_Bool sno_run_file(sno_State* state, const char* const path, size_t path_length);

sno_API sno_no_return void sno_throw(sno_State* state, const sno_String* exception_msg);

#endif
