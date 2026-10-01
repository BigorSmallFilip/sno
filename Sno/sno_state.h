#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno.h"
#include "sno_string.h"
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



typedef struct sno_GlobalState {
	StringInterningTable string_table;
	size_t mem_allocated;
	size_t num_allocations;
} sno_GlobalState;

typedef struct sno_VMState {
	sno_GlobalState* state;
	ExceptionType exception_type;
	ExceptionJump* exception_jump;
	const IString* exception_message;
} sno_VMState;



sno_no_return void vm_throw(
	sno_VMState* vm,
	ExceptionType type,
	const char* const message,
	size_t message_length
);



#endif
