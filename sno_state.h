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

#define sno_MAX_STACK 256

typedef struct sno_State {
	sno_Value* stack;
	uint32_t stack_base;
	uint32_t stack_top;
	uint32_t stack_capacity;
	sno_StringInterningTable string_table;
	const sno_String* exception_msg;
	sno_ExceptionJump* exception_jump;
} sno_State;

sno_API sno_State* sno_create_state();
sno_API void sno_reserve_stack(sno_State* state, uint32_t slots);
sno_API void sno_free_state(sno_State* state);

sno_API void sno_print_exception_msg(const sno_State* state);

sno_API sno_Bool sno_try_compile_source_code(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
);

sno_API sno_no_return void sno_throw(sno_State* state, const sno_String* exception_msg);

#endif
