#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno_utility.h"
#include "sno_string.h"
#include "sno_value.h"
#include <setjmp.h>
#include <stdarg.h>

typedef enum sno_ExceptionType {
	sno_EXCEPTION_NONE,
	sno_EXCEPTION_SYNTAX_ERROR,
	sno_EXCEPTION_RUNTIME_ERROR,
} sno_ExceptionType;

typedef struct sno_ExceptionJump {
	struct sno_ExceptionJump* prev;
	jmp_buf buf;
} sno_ExceptionJump;

typedef struct sno_CallInfo {
	uint32_t base;
	uint8_t num_args;
	uint8_t num_returns;
	sno_Instruction* saved_pc;
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

	sno_ExceptionType exception_type;
	const sno_IString* exception_msg;
	sno_ExceptionJump* exception_jump;

	sno_Table* globals;
	sno_Table* string_prototype;
	sno_Table* array_prototype;
	sno_Table* table_prototype;

	sno_GCObject* gc_list_start;
	size_t num_non_string_gc_objects;
	size_t memory_allocated;
	size_t num_allocations;
	size_t live_memory_last_gc;
} sno_State;

sno_API sno_State* sno_create_state();
sno_API void sno_free_state(sno_State* state);



sno_API sno_Value* sno_reserve_stack(sno_State* state, uint32_t slots);
sno_API sno_Value* sno_get_stack_ptr(sno_State* state, uint32_t slot);

sno_API void sno_check_arg_count(sno_State* state, uint8_t num_args, uint8_t num_args_expected);

#define sno_self (-1)
sno_API sno_Value* sno_get_arg(sno_State* state, int arg);
sno_API sno_Value* sno_get_arg_typed(
	sno_State* state,
	sno_ValueType expected_type,
	int arg
);

sno_API void sno_set_ret_none(sno_State* state, int ret);
sno_API void sno_set_ret_bool(sno_State* state, int ret, sno_Bool b);
sno_API void sno_set_ret_number(sno_State* state, int ret, sno_Number number);
sno_API void sno_set_ret_string(sno_State* state, int ret, const sno_IString* string);
sno_API void sno_set_ret_array(sno_State* state, int ret, sno_Array* arr);
sno_API void sno_set_ret_table(sno_State* state, int ret, sno_Table* table);

sno_API void sno_s_array_push(sno_State* state, uint32_t i);



sno_API void sno_create_new_global(sno_State* state, const sno_IString* name, const sno_Value* value);
sno_API void sno_set_global(sno_State* state, const sno_IString* name, const sno_Value* value);
sno_API void sno_get_global(sno_State* state, const sno_IString* name, const sno_Value* out_value);

sno_API void sno_print_globals(const sno_State* state);
sno_API void sno_print_exception_msg(const sno_State* state);

sno_API sno_Bool sno_try_compile_source_code(
	sno_State* state,
	const sno_IString* name,
	const sno_IString* source_code
);

sno_API void sno_call(sno_State* state, uint8_t num_args, uint8_t num_returns);
sno_API sno_Bool sno_run_file(sno_State* state, const char* const path, size_t path_length);



sno_API sno_no_return void sno_throw(
	sno_State* state,
	sno_ExceptionType exception_type,
	const char* const exception_msg,
	size_t exception_msg_len
);

sno_API sno_no_return void sno_throw_runtime_error(
	sno_State* state,
	const char* const format,
	...
);

sno_API sno_no_return void sno_throw_runtime_error_va(
	sno_State* state,
	const char* const format,
	va_list args
);

sno_API sno_no_return void sno_throw_at_source_code_pos(
	sno_State* state,
	sno_ExceptionType exception_type,
	const sno_IString* source_code,
	const sno_IString* source_code_name,
	uint32_t source_code_pos,
	const char* const format,
	va_list args
);

sno_API sno_no_return void sno_throw_at_source_code_pos_open_close(
	sno_State* state,
	sno_ExceptionType exception_type,
	const sno_IString* source_code,
	const sno_IString* source_code_name,
	uint32_t source_code_pos_open,
	uint32_t source_code_pos_close,
	const char* const format,
	va_list args
);

double sno_perftimer();

#endif
