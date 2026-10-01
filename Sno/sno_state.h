#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno.h"
#include <setjmp.h>



typedef enum ExceptionType {
	EXCEPTION_NONE,
	EXCEPTION_SYNTAX_ERROR,
	EXCEPTION_RUNTIME_ERROR,
	NUM_EXCEPTION_TYPES,
} ExceptionType;

typedef struct ExceptionJump {
	struct ExceptionJump* prev;
	jmp_buf buf;
} ExceptionJump;

#define EXCEPTION_MESSAGE_MAX_LENGTH 512



typedef struct sno_State {
	size_t mem_allocated;
	size_t num_allocations;
} sno_State;

typedef struct sno_VM {
	sno_State* state;
	ExceptionType exception_type;
	ExceptionJump* exception_jump;
	char exception_message[EXCEPTION_MESSAGE_MAX_LENGTH];
	size_t exception_message_length;
} sno_VM;



sno_no_return void sno_throw(
	sno_VM* vm,
	ExceptionType type,
	const char* const message,
	size_t message_length
);



#endif
