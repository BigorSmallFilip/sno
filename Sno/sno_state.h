#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno.h"
#include "sno_string.h"
#include "sno_value.h"
#include <setjmp.h>



// Simple index of a character in some source code string
typedef uint32_t SourceCodePos;

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

typedef struct sno_State {
	Value* stack;
	Value* stack_top;
	uint32_t stack_capacity;

	Table globals;

	StringInterningTable string_table;
	size_t mem_allocated;
	size_t num_allocations;

	ExceptionType exception_type;
	ExceptionJump* exception_jump;
	const IString* exception_message;
} sno_State;

typedef struct CallInfo {
	union {
		struct Bytecode* bytecode;
		sno_ForeignFunction* foreign;
	} function;
	sno_Bool is_c_function;

} CallInfo;



sno_no_return void state_throw(
	sno_State* state,
	ExceptionType type,
	const char* const message,
	size_t message_length
);



#endif
