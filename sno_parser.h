#ifndef sno_PARSER_H
#define sno_PARSER_H

#include "sno_utility.h"

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
	sno_TK_IDENTIFIER,

	sno_NUM_TOKENS,
	sno_TK_EOF = -1,
	sno_TK_ERROR = -2,
};
typedef int8_t sno_TokenType;

extern const char* const sno_token_strings[sno_NUM_TOKENS];

#define sno_token_is_assignment(tokentype) ((tokentype) >= sno_TK_ASSIGN && (tokentype) <= sno_TK_ASSIGNSHR)

#endif
