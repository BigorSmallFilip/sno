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

typedef struct sno_State {
	sno_StringInterningTable string_table;
	sno_String* exception_msg;
	sno_ExceptionJump* exception_jump;
} sno_State;

sno_API sno_State* sno_create_state();
sno_API void sno_free_state(sno_State* state);

sno_API sno_no_return void sno_throw(sno_State* state, const sno_String* exception_msg);

#endif
