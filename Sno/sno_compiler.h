#ifndef sno_COMPILER_H
#define sno_COMPILER_H

#include "sno.h"
#include "sno_mem.h"
#include "sno_state.h"
#include "sno_vm.h"
#include <stdarg.h>

typedef enum TokenType {
	TK_TERMINATOR,

	TK_IF,
	TK_ELSE,
	TK_FOR,
	TK_IN,
	TK_WHILE,
	TK_BREAK,
	TK_CONTINUE,
	TK_FUNCTION,
	TK_RETURN,
	TK_VAR,
	TK_CONST,
	TK_SELF,
	TK_TRUE,
	TK_FALSE,
	TK_NONE,
	TK_VEC2,
	TK_VEC3,
	TK_VEC4,
	TK_QUAT,
	TK_MAT2,
	TK_MAT3,
	TK_MAT4,

	TK_ADD,
	TK_SUB,
	TK_MUL,
	TK_DIV,
	TK_IDIV,
	TK_POW,
	TK_MOD,
	TK_BAND,
	TK_BOR,
	TK_BXOR,
	TK_SHL,
	TK_SHR,

	TK_ASSIGN,
	TK_ASSIGNADD,
	TK_ASSIGNSUB,
	TK_ASSIGNMUL,
	TK_ASSIGNDIV,
	TK_ASSIGNIDIV,
	TK_ASSIGNPOW,
	TK_ASSIGNMOD,
	TK_ASSIGNBAND,
	TK_ASSIGNBOR,
	TK_ASSIGNBXOR,
	TK_ASSIGNSHL,
	TK_ASSIGNSHR,

	TK_INC,
	TK_DEC,

	TK_BITFLIP,
	TK_LNOT,
	TK_LAND,
	TK_LOR,
	TK_EQ,
	TK_NEQ,
	TK_LT,
	TK_GT,
	TK_LE,
	TK_GE,

	TK_LPAREN,
	TK_RPAREN,
	TK_LBRACKET,
	TK_RBRACKET,
	TK_LBRACE,
	TK_RBRACE,
	TK_DOT,
	TK_COMMA,
	TK_COLON,

	TK_NUMBER,
	TK_STRING,
	TK_INTERPOLATED_STRING,
	TK_IDENTIFIER,

	NUM_TOKEN_TYPES,
	TK_EOF = -1,
} TokenType;
//typedef int8_t TokenType;

extern const char* const token_strings[NUM_TOKEN_TYPES];

#define token_is_assignment(tokentype) \
	((tokentype) >= TK_ASSIGN && (tokentype) <= TK_ASSIGNSHR)

typedef struct Token {
	TokenType type;
	uint32_t pos; // Offset in the source code string
	union {
		sno_Number number;
		struct IString* string;
	} info;
} Token;

void print_token(const Token* token);

#define MAX_SOURCE_CODE_LENGTH (UINT32_MAX - 69)
#define NO_POS (UINT32_MAX)
#define MAX_SYNTAX_DEPTH 200

typedef struct Tokenizer {
	Token token;
	Token prev_token;
	sno_Bool insert_terminator;
	sno_Bool next_token_is_comma;
	struct sno_VMState* parent_vm;
	struct IString* source_code_name;
	struct IString* source_code;
	const char* source_code_end;
	const char* cur_char;
	const char* token_start;
	uint8_t string_interpolation_depth;
	uint32_t syntax_depth;
	struct Compiler* cs;
} Tokenizer;

void read_first_token(Tokenizer* ts);
void read_next_token(Tokenizer* ts);

sno_Bool print_source_code_tokens(
	struct sno_VMState* vm,
	struct IString* source_code_name,
	struct IString* source_code
);



#define MAX_LOCAL_VARS_PER_FUNCTION 65000
#define MAX_ACTIVE_LOCAL_VARS 200

#define MAX_EXPR_PER_STMT 15
#if MAX_ACTIVE_LOCAL_VARS + MAX_EXPR_PER_STMT + 1 > 254
#error Too many local variables to store in one byte
#endif
#define MAX_CONCATS 64
#define MAX_NUMBER_CONSTANTS 65000
#define MAX_STRING_CONSTANTS 65000
#define MAX_FUNCTION_CONSTANTS 65000
#define MAX_STACK_CONSTRUCTOR_ARGS 200

DECLARE_GENERIC_DYN_ARRAY(SourceCodePos, SourceCodePos, pos);
DECLARE_GENERIC_DYN_ARRAY(PC, PC, pc);
DECLARE_GENERIC_DYN_ARRAY(Bytecode*, Bytecode, bytecode);
DECLARE_GENERIC_DYN_ARRAY(LocalVar, LocalVar, local_var);

#define MAX_BLOCK_DEPTH 3

typedef struct Block {
	struct Block* prev;
	uint8_t num_active_local_vars;
	sno_Bool is_loop;
	sno_Bool is_global;
	PCDynArray breaks_and_continues;
} Block;

typedef struct Compiler {
	Tokenizer* ts;
	IString* name;
	struct Compiler* parent_function;
	SourceCodePosDynArray instruction_pos;
	ByteDynArray instructions;
	NumberDynArray number_constants;
	IStringDynArray string_constants;
	BytecodeDynArray bytecode_constants;
	LocalVarDynArray local_vars;
	Block* current_block;
	size_t current_block_depth;
	PC last_instruction_pc;
	sno_Bool has_self_parameter;
	LocalSlot num_active_local_slots;
	LocalSlot max_active_local_slots;
	LocalID active_local_vars[MAX_ACTIVE_LOCAL_VARS]; // Indexes into the local_vars dynarray
} Compiler;

Bytecode* compile_source_code(
	sno_VMState* vm,
	struct IString* source_code_name,
	struct IString* source_code
);



size_t sprint_source_code_context(
	char* buffer,
	size_t buffer_size,
	struct IString* source_code,
	uint32_t pos
);

sno_no_return void syntax_error(
	Tokenizer* ts,
	uint32_t pos,
	const char* const message_format,
	...
);

sno_no_return void vsyntax_error(
	Tokenizer* ts,
	uint32_t pos,
	const char* const message_format,
	va_list args
);

#endif
