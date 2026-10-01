#ifndef sno_COMPILER_H
#define sno_COMPILER_H

#include "sno.h"

enum {
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
};
typedef int8_t TokenType;

extern const char* const token_strings[NUM_TOKEN_TYPES];

#define token_is_assignment(tokentype) \
	((tokentype) >= TK_ASSIGN && (tokentype) <= TK_ASSIGNSHR)

typedef struct Token {
	TokenType type;
	uint32_t pos; // Offset in the source code string
	union {
		sno_Number number;
		const struct IString* string;
	} info;
} Token;

void print_token(const Token* token);

#define MAX_SYNTAX_DEPTH 200

typedef struct Tokenizer {
	Token token;
	Token prev_token;
	sno_Bool insert_terminator;
	struct sno_VMState* parent_vm;
	const struct IString* source_code_name;
	const struct IString* source_code;
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
	const struct IString* source_code_name,
	const struct IString* source_code
);

#define MAX_LOCAL_VARS_PER_FUNCTION 65000
#define MAX_ACTIVE_LOCAL_VARS 200
#define MAX_STACK_ARGS 14
#if MAX_ACTIVE_LOCAL_VARS + MAX_STACK_ARGS > 254
#error Too many local variables to store in one byte
#endif
#define MAX_NUMBER_CONSTANTS 65000
#define MAX_STRING_CONSTANTS 65000
#define MAX_FUNCTION_CONSTANTS 65000
#define MAX_STACK_CONSTRUCTOR_ARGS 200

typedef struct Compiler {

	Tokenizer* ts;
} Compiler;

#endif
