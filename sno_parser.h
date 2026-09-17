#ifndef sno_PARSER_H
#define sno_PARSER_H

#include "sno_utility.h"
#include "sno_mem.h"



enum {
	sno_TK_IF,
	sno_TK_ELSE,
	sno_TK_FOR,
	sno_TK_IN,
	sno_TK_WHILE,
	sno_TK_BREAK,
	sno_TK_CONTINUE,
	sno_TK_FUNCTION,
	sno_TK_RETURN,
	sno_TK_VAR,
	sno_TK_CONST,
	sno_TK_SELF,
	sno_TK_TRUE,
	sno_TK_FALSE,
	sno_TK_NONE,

	sno_TK_ADD,
	sno_TK_SUB,
	sno_TK_MUL,
	sno_TK_DIV,
	sno_TK_IDIV,
	sno_TK_POW,
	sno_TK_MOD,
	sno_TK_BAND,
	sno_TK_BOR,
	sno_TK_BXOR,
	sno_TK_SHL,
	sno_TK_SHR,

	sno_TK_ASSIGN,
	sno_TK_ASSIGNADD,
	sno_TK_ASSIGNSUB,
	sno_TK_ASSIGNMUL,
	sno_TK_ASSIGNDIV,
	sno_TK_ASSIGNIDIV,
	sno_TK_ASSIGNPOW,
	sno_TK_ASSIGNMOD,
	sno_TK_ASSIGNBAND,
	sno_TK_ASSIGNBOR,
	sno_TK_ASSIGNBXOR,
	sno_TK_ASSIGNSHL,
	sno_TK_ASSIGNSHR,

	sno_TK_INC,
	sno_TK_DEC,

	sno_TK_BITFLIP,
	sno_TK_LNOT,
	sno_TK_LAND,
	sno_TK_LOR,
	sno_TK_EQ,
	sno_TK_NEQ,
	sno_TK_LT,
	sno_TK_GT,
	sno_TK_LE,
	sno_TK_GE,

	sno_TK_LPAREN,
	sno_TK_RPAREN,
	sno_TK_LBRACKET,
	sno_TK_RBRACKET,
	sno_TK_LBRACE,
	sno_TK_RBRACE,
	sno_TK_DOT,
	sno_TK_COMMA,
	sno_TK_COLON,

	sno_TK_NUMBER,
	sno_TK_STRING,
	sno_TK_INTERPOLATED_STRING,
	sno_TK_IDENTIFIER,

	sno_NUM_TOKENS,
	sno_TK_EOF = -1,
	sno_TK_ERROR = -2,
};
typedef int8_t sno_TokenType;

extern const char* const sno_token_strings[sno_NUM_TOKENS];

#define sno_token_is_assignment(tokentype) ((tokentype) >= sno_TK_ASSIGN && (tokentype) <= sno_TK_ASSIGNSHR)



typedef struct sno_Token {
	sno_TokenType type;
	sno_Bool stmt_end;
	uint32_t source_code_pos;
	union {
		sno_Number u_number;
		const struct sno_String* u_string;
	} info;
} sno_Token;

void sno_print_token(const sno_Token* token);

typedef struct sno_Tokenizer {
	sno_Token token;
	sno_Token prev_token;
	struct sno_State* main_state;
	const struct sno_String* source_code_name;
	const struct sno_String* source_code;
	const char* source_code_end;
	const char* cur_char;
	const char* token_start;
	uint8_t string_interpolation_depth;
	struct sno_Compiler* cs;
} sno_Tokenizer;

#define sno_MAX_LOCAL_VARS_PER_FUNCTION 50000
#define sno_MAX_ACTIVE_LOCAL_VARS 200
#define sno_MAX_NUMBER_CONSTANTS 50000
#define sno_MAX_STRING_CONSTANTS 50000
#define sno_MAX_SUB_FUNCTIONS 50000
#define sno_MAX_STACK_ARGS 14
#define sno_MAX_STACK_CONSTRUCTOR_ARGS 64

#define sno_MAX_BLOCK_DEPTH 50
typedef struct sno_Block {
	struct sno_Block* prev;
	uint8_t num_active_local_vars;
	sno_Bool is_loop;
	sno_Bool is_global;
} sno_Block;

typedef struct sno_Compiler {
	sno_Tokenizer* ts;
	sno_DynArray instructions;
	sno_DynArray instruction_source_code_offsets;
	sno_DynArray local_vars;
	sno_DynArray number_constants;
	sno_DynArray string_constants;
	sno_DynArray sub_functions;
	struct sno_Bytecode* bytecode;
	struct sno_Compiler* parent;
	sno_Block* current_block;
	uint8_t current_block_depth;
	sno_LocalSlot num_active_local_var_slots;
	sno_LocalSlot max_active_local_var_slots; // Number of stack slots needed for local vars
	sno_LocalID active_local_vars[sno_MAX_ACTIVE_LOCAL_VARS]; // Indexes into the local_vars dynarray
	uint32_t max_stack_used;
	uint32_t current_stack_idx;
	sno_Bool is_global_scope;
	sno_Bool has_self_parameter;
} sno_Compiler;

void sno_read_initial_token(sno_Tokenizer* ts);
void sno_read_next_token(sno_Tokenizer* ts);
void sno_continue_interpolated_string(sno_Tokenizer* ts);



int sno_sprintf_source_code_pos(
	struct sno_State* state,
	char* buffer,
	int buffer_size,
	const struct sno_String* source_code,
	uint32_t source_code_pos
);



sno_no_return void sno_throw_syntax_error_at(
	sno_Tokenizer* ts,
	uint32_t pos,
	const char* format,
	...
);

sno_no_return void sno_throw_syntax_error_at_cur_token(
	sno_Tokenizer* ts,
	const char* format,
	...
);

sno_no_return void sno_throw_syntax_error_open_close(
	sno_Tokenizer* ts,
	uint32_t pos_open,
	uint32_t pos_close,
	const char* format,
	...
);



struct sno_Bytecode* sno_parse_source_code(
	struct sno_State* state,
	const struct sno_String* name,
	const struct sno_String* source_code
);
sno_Bool sno_print_source_code(
	struct sno_State* state,
	const struct sno_String* name,
	const struct sno_String* source_code
);

#endif
