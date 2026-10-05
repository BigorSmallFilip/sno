#include "sno_compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include "sno_string.h"

#define is_whitespace(c) ((c) == ' ' || (c) == '\t')
#define is_alpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && c <= 'Z') || (c) == '_')
#define is_digit(c) ((c) >= '0' && (c) <= '9')

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
	sno_assert(token->type < NUM_TOKEN_TYPES);
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
		case TK_IDENTIFIER: printf(
			ANSI_VARIABLE "%.*s",
			(unsigned int)token->info.string->length,
			istring_chars(token->info.string)
		); break;
		default: sno_unreachable; break;
		}
	}
#ifdef sno_USE_ANSI_COLOR
	printf(ANSI_NORMAL);
#endif
}



static sno_inline sno_Bool check_next(Tokenizer* ts, char c) {
	sno_assert(ts->cur_char <= ts->source_code_end);
	if (ts->cur_char == ts->source_code_end) {
		return sno_FALSE;
	}
	if (*ts->cur_char == c) {
		ts->cur_char++;
		return sno_TRUE;
	} else {
		return sno_FALSE;
	}
}

static sno_inline sno_Bool check_next_alphanumeric(Tokenizer* ts) {
	char c = *ts->cur_char;
	if (is_alpha(c) || is_digit(c)) {
		ts->cur_char++;
		return sno_TRUE;
	} else {
		return sno_FALSE;
	}
}



static void read_string_literal(
	Tokenizer* ts,
	Token* token,
	sno_Bool* interpolated
) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);
	sno_assert_ptr(interpolated);
	
	sno_VMState* vm = ts->parent_vm;
	ByteDynArray formatted_string = { 0 };
	// TODO: Add an init size for dyn array
	//byte_dyn_array_init(vm, &formatted_string, 1, 512);
	while (1) {
		ts->cur_char++;
		switch (*ts->cur_char) {
		case '\n': case '\r': case '\0': {
			syntax_error(
				ts,
				(SourceCodePos)(
					ts->token_start -
					istring_chars(ts->source_code)
				),
				"String is missing closing quotes '\"'"
			);
		} break;
		case '\t': {
			syntax_error(
				ts,
				(SourceCodePos)(
					ts->cur_char -
					istring_chars(ts->source_code)
				),
				"Strings cannot contain tabs"
			);
		} break;
		case '\\': {
			ts->cur_char++;
			char escapedchar;
			switch (*ts->cur_char) {
			case 'a':  escapedchar = '\a'; break;
			case 'b':  escapedchar = '\b'; break;
			case 'f':  escapedchar = '\f'; break;
			case 'n':  escapedchar = '\n'; break;
			case 'r':  escapedchar = '\r'; break;
			case 't':  escapedchar = '\t'; break;
			case 'v':  escapedchar = '\v'; break;
			case '\\': escapedchar = '\\'; break;
			case '\"': escapedchar = '\"'; break;
			//case '\'': escapedchar = '\''; break;
			case '0':  escapedchar = '\0'; break;
			case '(': {
				// Interpolated string
				ts->string_interpolation_depth++;
				*interpolated = sno_TRUE;
				goto endstring;
			}
			default:
				syntax_error(
					ts,
					(SourceCodePos)(
						ts->cur_char -
						istring_chars(ts->source_code) - 1
					),
					"Invalid string escape character '\\%c'",
					*ts->cur_char
				);
				break;
			}
			byte_dyn_array_push(vm, &formatted_string, (uint8_t*)&escapedchar);
		} break;
		case '\"': {
			goto endstring;
		}

		default:
			// TODO: This is terribly inefficient
			// Do multiple chars at the same time dork
			byte_dyn_array_push(vm, &formatted_string, (uint8_t*)ts->cur_char);
			break;
		}
	}
endstring:
	ts->cur_char++; // Skip the closing double quotes
	token->info.string = create_istring(
		vm->state,
		(const char*)formatted_string.buffer,
		formatted_string.count
	);
	byte_dyn_array_clear(vm, &formatted_string);
}

static void read_base10_number(Tokenizer* ts, Token* token) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);
	sno_Bool has_decimal = sno_FALSE;
	if (ts->cur_char[0] == '0') {
		sno_assert(ts->cur_char[1] == '.');
	} else {
		sno_assert(is_digit(ts->cur_char[0]));
	}
	while (1) {
		char c = *ts->cur_char;
		if (c == '.') {
			if (has_decimal) {
				syntax_error(
					ts,
					(uint32_t)(ts->cur_char - istring_chars(ts->source_code)),
					"There are multiple decimal points in this number"
				);
			}
			has_decimal = sno_TRUE;
			ts->cur_char++;
			sno_assert(ts->cur_char <= ts->source_code_end);
			if (
				ts->cur_char == ts->source_code_end ||
				!is_digit(*ts->cur_char)
			) {
				syntax_error(
					ts,
					(uint32_t)(ts->cur_char - 1 - istring_chars(ts->source_code)),
					"Numbers can't end with a decimal point"
				);
			}
		} else if (is_alpha(c)) {
			syntax_error(
				ts,
				(uint32_t)(ts->cur_char - istring_chars(ts->source_code)),
				"There is a letter character in this number"
			);
		} else if (!is_digit(c)) {
			break;
		}
		ts->cur_char++;
		sno_assert(ts->cur_char <= ts->source_code_end);
		if (ts->cur_char == ts->source_code_end) {
			break;
		}
	}

	uint32_t length = (uint32_t)(ts->cur_char - ts->token_start);
	if (length >= 255) {
		syntax_error(
			ts,
			(uint32_t)(ts->token_start - istring_chars(ts->source_code)),
			"This number is way too long"
		);
	}
	token->info.number = strtod(ts->token_start, NULL);
}



static void read_comment(Tokenizer* ts) {
	sno_assert_ptr(ts);
	while (ts->cur_char < ts->source_code_end) {
		ts->cur_char++;
		if (*ts->cur_char == '\n' ||
			*ts->cur_char == '\r') break;
	}
}

static void read_multiline_comment(Tokenizer* ts) {
	sno_assert_ptr(ts);
	const char* start = ts->cur_char;
	while (1) {
		sno_assert(ts->cur_char <= ts->source_code_end);
		if (ts->cur_char == ts->source_code_end) {
			syntax_error(
				ts,
				(uint32_t)(start - istring_chars(ts->source_code) - 2),
				"This multi-line comment doesn't close"
			);
		}
		if (check_next(ts, '*')) {
			if (check_next(ts, '/')) {
				break;
			}
			continue;
		}
		if (check_next(ts, '/')) {
			if (check_next(ts, '*')) {
				// Nested multi-line comments
				read_multiline_comment(ts);
				// The multi-line comment function returns at the token AFTER the '*/'.
				// This means the next token shouldn't be skipped
				continue;
			}
			continue;
		}
		ts->cur_char++;
	}
}

static sno_Bool skip_whitespace_and_comments(
	Tokenizer* ts,
	sno_Bool insert_terminator_on_endline
) {
	int stmt_end = sno_FALSE;
	while (ts->cur_char != ts->source_code_end) {
		sno_assert(ts->cur_char < ts->source_code_end);
		switch (*ts->cur_char) {
		case '\n': {
			stmt_end = stmt_end || insert_terminator_on_endline;
			break;
		}
		case '\\': {
			const char* backslash = ts->cur_char;
			ts->cur_char++;
			if (check_next(ts, '\n') || (check_next(ts, '\r') && check_next(ts, '\n'))) {
				break;
			}
			syntax_error(
				ts,
				(uint32_t)(backslash - istring_chars(ts->source_code)),
				"Backslash characters must be the last character on a line, including spaces"
			);
			break;
		}
		case ' ': case '\t': {
			break;
		}
		case ';': {
			stmt_end = sno_TRUE;
			break;
		}
		case '/': {
			if (ts->cur_char + 1 == ts->source_code_end) {
				return stmt_end;
			}
			char next = *(ts->cur_char + 1);
			if (next == '/') {
				ts->cur_char += 2;
				read_comment(ts);
				stmt_end = stmt_end || insert_terminator_on_endline;
			} else if (next == '*') {
				ts->cur_char += 2;
				read_multiline_comment(ts);
			} else {
				return stmt_end;
			}
			continue;
		}
		default: {
			return stmt_end;
		}
		}
		ts->cur_char++;
	}
	return sno_TRUE;
}



static TokenType lex_token(Tokenizer* ts, Token* token) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);

	ts->token_start = ts->cur_char;
	token->pos = (uint32_t)(ts->token_start - istring_chars(ts->source_code));

	sno_assert(ts->cur_char <= ts->source_code_end);
	if (ts->cur_char == ts->source_code_end) {
		return TK_EOF;
	}

	switch (*ts->cur_char) {
	case '+': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNADD;
		else if (check_next(ts, '+')) return TK_INC;
		else return TK_ADD;
	}
	case '-': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNSUB;
		else if (check_next(ts, '-')) return TK_DEC;
		else return TK_SUB;
	}
	case '*': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNMUL;
		else if (check_next(ts, '*')) {
			if (check_next(ts, '=')) return TK_ASSIGNPOW;
			else return TK_POW;
		} else return TK_MUL;
	}
	case '/': {
		ts->cur_char++;
		sno_assert(!check_next(ts, '/'));
		sno_assert(!check_next(ts, '*'));
		if (check_next(ts, '=')) return TK_ASSIGNDIV;
		else if (check_next(ts, '-')) {
			if (check_next(ts, '=')) return TK_ASSIGNIDIV;
			else return TK_IDIV;
		} else return TK_DIV;
	}
	case '%': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNMOD;
		else return TK_MOD;
	}
	case '&': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNBAND;
		else if (check_next(ts, '&')) return TK_LAND;
		else return TK_BAND;
	}
	case '|': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNBOR;
		else if (check_next(ts, '|')) return TK_LOR;
		else return TK_BOR;
	}
	case '^': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_ASSIGNBXOR;
		else return TK_BXOR;
	}
	case '<': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_LE;
		else if (check_next(ts, '<')) {
			if (check_next(ts, '=')) return TK_ASSIGNSHL;
			else return TK_SHL;
		} else return TK_LT;
	}
	case '>': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_GE;
		else if (check_next(ts, '>')) {
			if (check_next(ts, '=')) return TK_ASSIGNSHR;
			else return TK_SHR;
		} else return TK_GT;
	}
	case '=': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_EQ;
		else return TK_ASSIGN;
	}
	case '~': {
		ts->cur_char++;
		return TK_BITFLIP;
	}
	case '!': {
		ts->cur_char++;
		if (check_next(ts, '=')) return TK_NEQ;
		else return TK_LNOT;
	}
	case '(': {
		ts->cur_char++;
		return TK_LPAREN;
	}
	case ')': {
		ts->cur_char++;
		return TK_RPAREN;
	}
	case '[': {
		ts->cur_char++;
		return TK_LBRACKET;
	}
	case ']': {
		ts->cur_char++;
		return TK_RBRACKET;
	}
	case '{': {
		ts->cur_char++;
		return TK_LBRACE;
	}
	case '}': {
		ts->cur_char++;
		return TK_RBRACE;
	}
	case '.': {
		ts->cur_char++;
		return TK_DOT;
	}
	case ',': {
		ts->cur_char++;
		return TK_COMMA;
	}
	case ':': {
		ts->cur_char++;
		return TK_COLON;
	}
	case '\"': {
		sno_Bool interpolated = sno_FALSE;
		read_string_literal(ts, token, &interpolated);
		return TK_STRING + interpolated;
	}

	case '0': {
		ts->cur_char++;
		if (check_next(ts, '.')) {
			read_base10_number(ts, token);
			return TK_NUMBER;
		} else if (check_next(ts, 'x')) {
			sno_not_implemented;
		} else if (check_next(ts, 'b')) {
			sno_not_implemented;
		} else if (!is_digit(*ts->cur_char)) {
			token->info.number = 0;
			return TK_NUMBER;
		} else {
			syntax_error(
				ts,
				(uint32_t)(ts->token_start - istring_chars(ts->source_code)),
				"Numbers cannot start with leading 0s"
			);
		}
	}
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9': {
		read_base10_number(ts, token);
		return TK_NUMBER;
	}

	case 'a': {
		ts->cur_char++;
		if (!check_next(ts, 'n')) goto identifier;
		if (!check_next(ts, 'd')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_LAND;
	}
	case 'b': {
		ts->cur_char++;
		if (!check_next(ts, 'r')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (!check_next(ts, 'a')) goto identifier;
		if (!check_next(ts, 'k')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_BREAK;
	}
	case 'c': {
		ts->cur_char++;
		if (!check_next(ts, 'o')) goto identifier;
		if (!check_next(ts, 'n')) goto identifier;
		if (check_next(ts, 's')) {
			if (!check_next(ts, 't')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_CONST;
		} else if (check_next(ts, 't')) {
			if (!check_next(ts, 'i')) goto identifier;
			if (!check_next(ts, 'n')) goto identifier;
			if (!check_next(ts, 'u')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_CONTINUE;
		}
		goto identifier;
	}
	case 'e': {
		ts->cur_char++;
		if (!check_next(ts, 'l')) goto identifier;
		if (!check_next(ts, 's')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_ELSE;
	}
	case 'f': {
		ts->cur_char++;
		if (check_next(ts, 'a')) {
			if (!check_next(ts, 'l')) goto identifier;
			if (!check_next(ts, 's')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_FALSE;
		} else if (check_next(ts, 'o')) {
			if (!check_next(ts, 'r')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_FOR;
		} else if (check_next(ts, 'u')) {
			if (!check_next(ts, 'n')) goto identifier;
			if (!check_next(ts, 'c')) goto identifier;
			if (!check_next(ts, 't')) goto identifier;
			if (!check_next(ts, 'i')) goto identifier;
			if (!check_next(ts, 'o')) goto identifier;
			if (!check_next(ts, 'n')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_FUNCTION;
		}
		goto identifier;
	}
	case 'i': {
		ts->cur_char++;
		if (check_next(ts, 'f')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_IF;
		} else if (check_next(ts, 'n')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_IN;
		}
		goto identifier;
	}
	case 'm': {
		ts->cur_char++;
		if (!check_next(ts, 'a')) goto identifier;
		if (!check_next(ts, 't')) goto identifier;
		if (check_next(ts, '2')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_MAT2;
		}
		if (check_next(ts, '3')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_MAT3;
		}
		if (check_next(ts, '4')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_MAT4;
		}
		goto identifier;
	}
	case 'n': {
		ts->cur_char++;
		if (check_next(ts, 'o')) {
			if (!check_next(ts, 'n')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_NONE;
		} else if (check_next(ts, 't')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_LNOT;
		}
	}
	case 'o': {
		ts->cur_char++;
		if (!check_next(ts, 'r')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_LOR;
	}
	case 'q': {
		ts->cur_char++;
		if (!check_next(ts, 'u')) goto identifier;
		if (!check_next(ts, 'a')) goto identifier;
		if (!check_next(ts, 't')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_QUAT;
	}
	case 'r': {
		ts->cur_char++;
		if (!check_next(ts, 'e')) goto identifier;
		if (!check_next(ts, 't')) goto identifier;
		if (!check_next(ts, 'u')) goto identifier;
		if (!check_next(ts, 'r')) goto identifier;
		if (!check_next(ts, 'n')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_RETURN;
	}
	case 's': {
		ts->cur_char++;
		if (!check_next(ts, 'e')) goto identifier;
		if (!check_next(ts, 'l')) goto identifier;
		if (!check_next(ts, 'f')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_SELF;
	}
	case 't': {
		ts->cur_char++;
		if (!check_next(ts, 'r')) goto identifier;
		if (!check_next(ts, 'u')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_TRUE;
	}
	case 'v': {
		ts->cur_char++;
		if (check_next(ts, 'a')) {
			if (!check_next(ts, 'r')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return TK_VAR;
		} else if (check_next(ts, 'e')) {
			if (!check_next(ts, 'c')) goto identifier;
			if (check_next(ts, '2')) {
				if (check_next_alphanumeric(ts)) goto identifier;
				return TK_VEC2;
			}
			if (check_next(ts, '3')) {
				if (check_next_alphanumeric(ts)) goto identifier;
				return TK_VEC3;
			}
			if (check_next(ts, '4')) {
				if (check_next_alphanumeric(ts)) goto identifier;
				return TK_VEC4;
			}
		}
		goto identifier;
	}
	case 'w': {
		ts->cur_char++;
		if (!check_next(ts, 'h')) goto identifier;
		if (!check_next(ts, 'i')) goto identifier;
		if (!check_next(ts, 'l')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return TK_WHILE;
	}

	                              case 'd':
	case 'g': case 'h': case 'j': case 'k': case 'l':
	                              case 'p':
	case 'u':                     case 'x':
	case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
	case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
	case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R':
	case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
	case 'Y': case 'Z': case '_':
	identifier:
	{
		if (is_alpha(*ts->cur_char) || is_digit(*ts->cur_char)) {
			ts->cur_char++;
			goto identifier;
		}
		size_t length = ts->cur_char - ts->token_start;
		sno_assert(length < UINT32_MAX);
		token->info.string = create_istring(
			ts->parent_vm->state,
			ts->token_start,
			length
		);
		return TK_IDENTIFIER;
	}

	default: {
		// All other characters are invalid
		syntax_error(
			ts,
			(uint32_t)(ts->token_start - istring_chars(ts->source_code)),
			"There is an invalid character here"
		);
	}
	}
	//sno_unreachable; // Gives a warning about unreachable code
}



void read_first_token(Tokenizer* ts) {
	sno_assert_ptr(ts);
	(void)skip_whitespace_and_comments(ts, sno_FALSE);
	ts->token.type = TK_EOF; // Will become prev_token
	ts->insert_terminator = sno_FALSE;
	read_next_token(ts);
}

void read_next_token(Tokenizer* ts) {
	sno_assert_ptr(ts);
	ts->prev_token = ts->token;
	if (ts->insert_terminator) {
		ts->insert_terminator = sno_FALSE;
		ts->token.type = TK_TERMINATOR;
	} else {
		ts->token.type = lex_token(ts, &ts->token);
		sno_Bool insert_terminator_on_endline = sno_FALSE;
		switch (ts->token.type) {
		case TK_IDENTIFIER:
		case TK_FALSE:
		case TK_TRUE:
		case TK_NONE:
		case TK_NUMBER:
		case TK_STRING:
		case TK_BREAK:
		case TK_CONTINUE:
		case TK_RETURN:
		case TK_RPAREN:
		case TK_RBRACKET:
		case TK_RBRACE:
			insert_terminator_on_endline = sno_TRUE;
		default: break;
		}
		ts->insert_terminator = skip_whitespace_and_comments(
			ts,
			insert_terminator_on_endline
		);
	}
}



static void print_source_code_throws(
	sno_VMState* vm,
	IString* source_code_name,
	IString* source_code
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(source_code_name);
	sno_assert_ptr(source_code);

	Tokenizer ts = { 0 };
	ts.parent_vm = vm;
	ts.source_code_name = source_code_name;
	ts.source_code = source_code;
	ts.source_code_end = istring_chars(source_code) + source_code->length;
	ts.cur_char = istring_chars(source_code);
	ts.token_start = istring_chars(source_code);
	ts.cs = NULL;

	read_first_token(&ts);
	sno_Bool new_stmt = sno_TRUE;
	while (1) {
		if (new_stmt) {
			printf("stmt | ");
		}
		print_token(&ts.token);
		if (ts.token.type == TK_TERMINATOR) {
			putchar('\n');
		} else {
			putchar(' ');
		}
		new_stmt = ts.token.type == TK_TERMINATOR;

		read_next_token(&ts);
		if (ts.token.type < 0) {
			break;
		}
	}
	putchar('\n');
	putchar('\n');
}

sno_Bool print_source_code_tokens(
	sno_VMState* vm,
	IString* source_code_name,
	IString* source_code
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(source_code_name);
	sno_assert_ptr(source_code);

	sno_Bool success = sno_TRUE;
	ExceptionJump exception_jump;
	exception_jump.prev = vm->exception_jump;
	vm->exception_jump = &exception_jump;
	if (setjmp(exception_jump.buf) == EXCEPTION_NONE) {
		print_source_code_throws(vm, source_code_name, source_code);
	} else {
		fprintf(stderr, "\n");
		sno_print_error_message(vm);
		success = sno_FALSE;
	}
	vm->exception_jump = vm->exception_jump->prev;
	return success;
}





static size_t sprint_syntax_error_header(
	char* buffer,
	size_t buffer_length,
	const IString* source_code_name
) {
	sno_assert_ptr(buffer);
	sno_assert_ptr(source_code_name);
	return (size_t)snprintf(
		buffer,
		buffer_length,
		sno_ANSI_RED "Syntax error in %.*s" ANSI_NORMAL "\n",
		(unsigned int)source_code_name->length,
		istring_chars(source_code_name)
	);
}

static const char* find_line_start(const char* string, const char* view) {
	const char* p = view;
	while (p >= string) {
		if (*p == '\n') {
			return p + 1;
		}
		p--;
	}
	return string;
}

static const char* find_first_non_whitespace_char_on_line(
	const char* line_start,
	const char* string_end
) {
	const char* p = line_start;
	while (p < string_end) {
		if (!is_whitespace(*p)) {
			break;
		}
		p++;
	}
	return p;
}

static size_t find_line_number(const char* string, const char* view) {
	size_t linenum = 1;
	for (const char* i = string; i < view; i++) {
		if (*i == '\n') {
			linenum++;
		}
	}
	return linenum;
}

static size_t underline_token(
	char* buffer,
	size_t buffer_size,
	const IString* source_code,
	uint32_t pos
) {
	sno_assert_ptr(buffer);
	sno_assert_ptr(source_code);
	sno_assert(pos < source_code->length);

	uint32_t underline_length = 1;
	const char* p = istring_chars(source_code) + pos;
	const char* source_code_end =
		istring_chars(source_code) + source_code->length;
	if (is_alpha(*p)) {
		p++;
		for (; p < source_code_end; p++) {
			if (is_alpha(*p) || is_digit(*p)) {
				underline_length++;
			} else {
				break;
			}
		}
	}

	size_t length = 0;
	if (underline_length == 1) {
		length += snprintf(
			buffer + length,
			buffer_size - length,
			"^ "
		);
	} else {
		for (size_t i = 0; i < underline_length; i++) {
			buffer[length++] = '~';
		}
		buffer[length++] = ' ';
	}
	
	return length;
}

size_t sprint_source_code_context(
	char* buffer,
	size_t buffer_size,
	IString* source_code,
	uint32_t pos
) {
	const char* string = istring_chars(source_code);
	size_t string_length = source_code->length;
	const char* string_end = string + string_length;
	sno_assert(pos <= string_length);
	const char* string_pos = string + pos;

	const char* line_start = find_line_start(string, string_pos);
	size_t line = find_line_number(string, line_start);
	const char* p = find_first_non_whitespace_char_on_line(line_start, string_end);

	size_t length = 0;
	size_t spaces_before_pos = 0;
	length += snprintf(buffer + length, buffer_size - length, sno_ANSI_CYAN "        |  \n");
	length += snprintf(buffer + length, buffer_size - length, " %5u  |  " sno_ANSI_NORMAL, (unsigned int)line);
	while (p < string_pos) {
		if (*p == '\t') {
			spaces_before_pos &= ~(3);
			spaces_before_pos += 4;
			length &= ~(3); // TODO: This will remove already printed text??
			buffer[length++] = ' ';
			buffer[length++] = ' ';
			buffer[length++] = ' ';
			buffer[length++] = ' ';
		} else {
			spaces_before_pos++;
			buffer[length++] = *p;
		}
		p++;
	}
	while (p < string_end) {
		if (*p == '\n') {
			break;
		}
		buffer[length++] = *(p++);
	}
	length += snprintf(buffer + length, buffer_size - length, "\n" sno_ANSI_CYAN "        |  " sno_ANSI_RED);
	for (size_t i = 0; i < spaces_before_pos; i++) {
		buffer[length++] = ' ';
	}
	length += underline_token(buffer + length, buffer_size - length, source_code, pos);
	return length;
}



sno_no_return void syntax_error_args(
	Tokenizer* ts,
	SourceCodePos pos,
	const char* const message_format,
	va_list args
) {
	sno_assert_ptr(ts);
	sno_assert_ptr(pos < ts->source_code->length);

	char buffer[sno_STACK_BUFFER_LENGTH];
	size_t length = 0;
	length += sprint_syntax_error_header(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		ts->source_code_name
	);
	length += sprint_source_code_context(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		ts->source_code,
		pos
	);
	length += (size_t)vsnprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		message_format,
		args
	);
	va_end(args);
	length += (size_t)snprintf(
		buffer + length,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		ANSI_NORMAL "\n"
	);
	vm_throw(ts->parent_vm, EXCEPTION_SYNTAX_ERROR, buffer, length);
}

sno_no_return void syntax_error(
	Tokenizer* ts,
	uint32_t pos,
	const char* const message_format,
	...
) {
	va_list args;
	va_start(args, message_format);
	syntax_error_args(ts, pos, message_format, args);
}

sno_no_return void vsyntax_error(
	Tokenizer* ts,
	uint32_t pos,
	const char* const message_format,
	va_list args
) {
	syntax_error_args(ts, pos, message_format, args);
}
