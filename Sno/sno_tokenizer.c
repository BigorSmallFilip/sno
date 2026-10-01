#include "sno_compiler.h"

#include <stdio.h>
#include "sno_state.h"
#include "sno_string.h"

const char* const token_strings[NUM_TOKEN_TYPES] = {
	";",
	"if",
	"else",
	"for",
	"in",
	"while",
	"break",
	"continue",
	"function",
	"return",
	"var",
	"const",
	"self",
	"true",
	"false",
	"none",
	"vec2",
	"vec3",
	"vec4",
	"quat",
	"mat2",
	"mat3",
	"mat4",
	"+",
	"-",
	"*",
	"/",
	"/-",
	"**",
	"%",
	"&",
	"|",
	"^",
	"<<",
	">>",
	"=",
	"+=",
	"-=",
	"*=",
	"/=",
	"/-=",
	"**=",
	"%=",
	"&=",
	"|=",
	"^=",
	"<<=",
	">>=",
	"++",
	"--",
	"~",
	"!",
	"&&",
	"||",
	"==",
	"!=",
	"<",
	">",
	"<=",
	">=",
	"(",
	")",
	"[",
	"]",
	"{",
	"}",
	".",
	",",
	":",
	"number",
	"string",
	"interpolated string",
	"identifier",
};

#ifdef sno_USE_ANSI_COLOR
#define ANSI_KEYWORD    "\x1B[38;2;216;160;223m"
#define ANSI_CONST      "\x1B[38;2;86;156;214m"
#define ANSI_VARIABLE   "\x1B[38;2;156;220;254m"
#define ANSI_TYPE       "\x1B[38;2;78;201;176m"
#define ANSI_FUNCTION   "\x1B[38;2;220;220;170m"
#define ANSI_NUMBER     "\x1B[38;2;181;206;168m"
#define ANSI_STRING     "\x1B[38;2;206;145;120m"
#define ANSI_OPERATOR   "\x1B[38;2;180;180;180m"
#define ANSI_SEPARATOR  "\x1B[38;2;218;218;218m"
#define ANSI_EOF        "\x1B[38;2;255;0;0m"
#define ANSI_WHITE      "\x1B[38;2;218;218;218m"
#define ANSI_BACKGROUND "\x1B[48;2;30;30;30m"
#define ANSI_NORMAL     "\x1B[0m"
#else
#define ANSI_KEYWORD   	""
#define ANSI_CONST     	""
#define ANSI_VARIABLE  	""
#define ANSI_TYPE      	""
#define ANSI_FUNCTION  	""
#define ANSI_NUMBER    	""
#define ANSI_STRING    	""
#define ANSI_OPERATOR  	""
#define ANSI_SEPARATOR 	""
#define ANSI_EOF       	""
#define ANSI_WHITE     	""
#define ANSI_BACKGROUND	""
#define ANSI_NORMAL    	""
#endif

void print_token(const Token* token) {
	sno_assert_ptr(token);
	if (token->type < 0) {
		printf(ANSI_EOF "(End of file)");
	} else {
		switch (token->type) {
		case TK_TERMINATOR: printf(ANSI_OPERATOR ";"); break;
		case TK_IF: printf(ANSI_KEYWORD "if"); break;
		case TK_ELSE:  printf(ANSI_KEYWORD "else"); break;
		case TK_FOR: printf(ANSI_KEYWORD "for"); break;
		case TK_IN: printf(ANSI_KEYWORD "in"); break;
		case TK_WHILE: printf(ANSI_KEYWORD "while"); break;
		case TK_BREAK: printf(ANSI_KEYWORD "break"); break;
		case TK_CONTINUE: printf(ANSI_KEYWORD "continue"); break;
		case TK_FUNCTION: printf(ANSI_CONST "function"); break;
		case TK_RETURN: printf(ANSI_KEYWORD "return"); break;
		case TK_VAR: printf(ANSI_CONST "var"); break;
		case TK_CONST: printf(ANSI_CONST "const"); break;
		case TK_SELF: printf(ANSI_CONST "self"); break;
		case TK_TRUE: printf(ANSI_CONST "true"); break;
		case TK_FALSE: printf(ANSI_CONST "false"); break;
		case TK_NONE: printf(ANSI_CONST "none"); break;
		case TK_VEC2: printf(ANSI_CONST "vec2"); break;
		case TK_VEC3: printf(ANSI_CONST "vec3"); break;
		case TK_VEC4: printf(ANSI_CONST "vec4"); break;
		case TK_QUAT: printf(ANSI_CONST "quat"); break;
		case TK_MAT2: printf(ANSI_CONST "mat2"); break;
		case TK_MAT3: printf(ANSI_CONST "mat3"); break;
		case TK_MAT4: printf(ANSI_CONST "mat4"); break;
		case TK_ADD: printf(ANSI_OPERATOR "+"); break;
		case TK_SUB: printf(ANSI_OPERATOR "-"); break;
		case TK_MUL: printf(ANSI_OPERATOR "*"); break;
		case TK_DIV: printf(ANSI_OPERATOR "/"); break;
		case TK_IDIV: printf(ANSI_OPERATOR "/-"); break;
		case TK_POW: printf(ANSI_OPERATOR "**"); break;
		case TK_MOD: printf(ANSI_OPERATOR "%%"); break;
		case TK_BAND: printf(ANSI_OPERATOR "&"); break;
		case TK_BOR: printf(ANSI_OPERATOR "|"); break;
		case TK_BXOR: printf(ANSI_OPERATOR "^"); break;
		case TK_SHL: printf(ANSI_OPERATOR "<<"); break;
		case TK_SHR: printf(ANSI_OPERATOR ">>"); break;
		case TK_ASSIGN: printf(ANSI_OPERATOR "="); break;
		case TK_ASSIGNADD: printf(ANSI_OPERATOR "+="); break;
		case TK_ASSIGNSUB: printf(ANSI_OPERATOR "-="); break;
		case TK_ASSIGNMUL: printf(ANSI_OPERATOR "*="); break;
		case TK_ASSIGNDIV: printf(ANSI_OPERATOR "/="); break;
		case TK_ASSIGNIDIV: printf(ANSI_OPERATOR "//="); break;
		case TK_ASSIGNPOW: printf(ANSI_OPERATOR "**="); break;
		case TK_ASSIGNMOD: printf(ANSI_OPERATOR "%%="); break;
		case TK_ASSIGNBAND: printf(ANSI_OPERATOR "&="); break;
		case TK_ASSIGNBOR: printf(ANSI_OPERATOR "|="); break;
		case TK_ASSIGNBXOR: printf(ANSI_OPERATOR "^="); break;
		case TK_ASSIGNSHL: printf(ANSI_OPERATOR "<<="); break;
		case TK_ASSIGNSHR: printf(ANSI_OPERATOR ">>="); break;
		case TK_INC: printf(ANSI_OPERATOR "++"); break;
		case TK_DEC: printf(ANSI_OPERATOR "--"); break;
		case TK_BITFLIP: printf(ANSI_OPERATOR "~"); break;
		case TK_LNOT: printf(ANSI_OPERATOR "!"); break;
		case TK_LAND: printf(ANSI_OPERATOR "&&"); break;
		case TK_LOR: printf(ANSI_OPERATOR "||"); break;
		case TK_EQ: printf(ANSI_OPERATOR "=="); break;
		case TK_NEQ: printf(ANSI_OPERATOR "!="); break;
		case TK_LT: printf(ANSI_OPERATOR "<"); break;
		case TK_GT: printf(ANSI_OPERATOR ">"); break;
		case TK_LE: printf(ANSI_OPERATOR "<="); break;
		case TK_GE: printf(ANSI_OPERATOR ">="); break;
		case TK_LPAREN: printf(ANSI_SEPARATOR "("); break;
		case TK_RPAREN: printf(ANSI_SEPARATOR ")"); break;
		case TK_LBRACKET: printf(ANSI_SEPARATOR "["); break;
		case TK_RBRACKET: printf(ANSI_SEPARATOR "]"); break;
		case TK_LBRACE: printf(ANSI_SEPARATOR "{"); break;
		case TK_RBRACE: printf(ANSI_SEPARATOR "}"); break;
		case TK_DOT: printf(ANSI_OPERATOR "."); break;
		case TK_COMMA: printf(ANSI_SEPARATOR ","); break;
		case TK_COLON: printf(ANSI_SEPARATOR ":"); break;
		case TK_NUMBER: printf(ANSI_NUMBER "%g", (double)token->info.number); break;
		case TK_STRING: printf(
			ANSI_STRING "\"%.*s\"",
			(unsigned int)token->info.string->length,
			istring_chars(token->info.string)
		); break;
		case TK_INTERPOLATED_STRING: printf(
			ANSI_STRING "\"%.*s\" " ANSI_CONST "\\()",
			(unsigned int)token->info.string->length,
			istring_chars(token->info.string)
		); break;
		case TK_IDENTIFIER:
		{
			/*const char* const str = string_chars(token->info.u_string);
			const size_t len = token->info.u_string->length;
			if (next_token && next_token->type == TK_LPAREN) {
				printf(ANSI_FUNCTION);
				goto print_the_thing;
			}
			if (((str[0] >= 'A' && str[0] <= 'Z') || str[0] == '_')) {
				printf(ANSI_TYPE);
				for (size_t i = 1; i < len; i++) {
					if (!((str[i] >= 'A' && str[i] <= 'Z') || str[i] == '_')) {
						goto print_the_thing;
					}
				}
				printf(ANSI_CONST);
			} else {
				printf(ANSI_VARIABLE);
			}
		print_the_thing:*/
			printf(
				ANSI_VARIABLE "%.*s",
				(unsigned int)token->info.string->length,
				istring_chars(token->info.string)
			);
			break;
		}
		default: sno_unreachable; break;
		}
	}
	printf(ANSI_NORMAL);
}
