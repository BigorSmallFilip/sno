#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno.h"
#include "sno_string.h"
#include "sno_value.h"
#include <setjmp.h>

#define INITIAL_STACK_SIZE (128)

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

typedef struct CallInfo {
	union {
		struct Bytecode* bytecode;
		sno_ForeignFunction* foreign;
	} function;
	sno_Bool is_foreign_function;
	uint8_t argc;
	uint32_t caller_base_idx;
} CallInfo;

DECLARE_GENERIC_DYN_ARRAY(CallInfo, CallInfo, call_info);

typedef struct sno_State {
	Value* stack;
	Value* stack_base;
	Value* stack_top;
	uint32_t stack_capacity;

	CallInfoDynArray call_stack;

	Table* globals;

	StringInterningSet string_set;
	size_t mem_allocated;
	size_t num_allocations;

	ExceptionType exception_type;
	ExceptionJump* exception_jump;
	const IString* exception_message;
} sno_State;

void create_new_global(sno_State* state, Value name, Value value);

sno_no_return void state_throw(
	sno_State* state,
	ExceptionType type,
	const char* const message,
	size_t message_length
);



#endif
