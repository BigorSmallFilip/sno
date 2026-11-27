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

typedef struct CallInfo {
	union {
		struct Bytecode* bytecode;
		struct ForeignFunction* foreign;
	} function;
	sno_Bool is_foreign_function;
	uint8_t argc;
	uint32_t caller_base_idx;
	uint8_t* pc;
} CallInfo;

DECLARE_GENERIC_DYN_ARRAY(CallInfo, CallInfo, call_info);

typedef struct sno_State {
	Value* stack;
	Value* stack_base;
	Value* stack_top;
	uint32_t stack_capacity;

	CallInfoDynArray call_stack;

	Table* globals;
	Table* string_prototype;
	Table* array_prototype;
	Table* table_prototype;

	StringInterningSet string_set;
	size_t mem_allocated;
	size_t num_allocations;

	ExceptionType exception_type;
	ExceptionJump* exception_jump;
	const IString* exception_message;

	const IString* count_builtin_string;

	uint64_t random_seed;
} sno_State;

extern const sno_Library lib_core[];
extern const sno_Library lib_math[];
extern const sno_Library lib_string[];
extern const sno_Library lib_array[];
extern const sno_Library lib_table[];



void ensure_stack_capacity(sno_State* state, uint32_t free_slots);

void check_arg_type(sno_State* state, uint32_t idx, ValueType type);
#define set_retc(retc) state->stack_top = state->stack_base + retc; return retc

void create_new_global(sno_State* state, Value name, Value value);

void print_stack(sno_State* state);
int sprint_call_stack(
	char* buffer,
	int buffer_length,
	sno_State* state
);

sno_no_return void state_throw(
	sno_State* state,
	ExceptionType type,
	const char* const message,
	size_t message_length
);

uint8_t state_call(sno_State* state, uint8_t argc, uint8_t retc, uint8_t* pc);



double perftimer(void);

#endif
