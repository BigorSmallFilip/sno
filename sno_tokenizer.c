#include "sno_parser.h"

#include "sno_state.h"
#include <string.h>
#include <stdarg.h>

#define is_whitespace(c) ((c) == ' ' || (c) == '\t')
#define is_alpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && c <= 'Z') || (c) == '_')
#define is_digit(c) ((c) >= '0' && (c) <= '9')

const char* const sno_token_strings[sno_NUM_TOKENS] = {
	"(terminator)"
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
	"interpolated string"
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

void sno_print_token(const sno_Token* token) {
	sno_assert_ptr(token);
	if (token->type < 0) {
		printf(ANSI_EOF "(End of file)");
	} else {
		switch (token->type) {
		case sno_TK_TERMINATOR: printf(ANSI_OPERATOR ";"); break;
		case sno_TK_IF: printf(ANSI_KEYWORD "if"); break;
		case sno_TK_ELSE:  printf(ANSI_KEYWORD "else"); break;
		case sno_TK_FOR: printf(ANSI_KEYWORD "for"); break;
		case sno_TK_IN: printf(ANSI_KEYWORD "in"); break;
		case sno_TK_WHILE: printf(ANSI_KEYWORD "while"); break;
		case sno_TK_BREAK: printf(ANSI_KEYWORD "break"); break;
		case sno_TK_CONTINUE: printf(ANSI_KEYWORD "continue"); break;
		case sno_TK_FUNCTION: printf(ANSI_CONST "function"); break;
		case sno_TK_RETURN: printf(ANSI_KEYWORD "return"); break;
		case sno_TK_VAR: printf(ANSI_CONST "var"); break;
		case sno_TK_CONST: printf(ANSI_CONST "const"); break;
		case sno_TK_SELF: printf(ANSI_CONST "self"); break;
		case sno_TK_TRUE: printf(ANSI_CONST "true"); break;
		case sno_TK_FALSE: printf(ANSI_CONST "false"); break;
		case sno_TK_NONE: printf(ANSI_CONST "none"); break;
		case sno_TK_ADD: printf(ANSI_OPERATOR "+"); break;
		case sno_TK_SUB: printf(ANSI_OPERATOR "-"); break;
		case sno_TK_MUL: printf(ANSI_OPERATOR "*"); break;
		case sno_TK_DIV: printf(ANSI_OPERATOR "/"); break;
		case sno_TK_IDIV: printf(ANSI_OPERATOR "/-"); break;
		case sno_TK_POW: printf(ANSI_OPERATOR "**"); break;
		case sno_TK_MOD: printf(ANSI_OPERATOR "%%"); break;
		case sno_TK_BAND: printf(ANSI_OPERATOR "&"); break;
		case sno_TK_BOR: printf(ANSI_OPERATOR "|"); break;
		case sno_TK_BXOR: printf(ANSI_OPERATOR "^"); break;
		case sno_TK_SHL: printf(ANSI_OPERATOR "<<"); break;
		case sno_TK_SHR: printf(ANSI_OPERATOR ">>"); break;
		case sno_TK_ASSIGN: printf(ANSI_OPERATOR "="); break;
		case sno_TK_ASSIGNADD: printf(ANSI_OPERATOR "+="); break;
		case sno_TK_ASSIGNSUB: printf(ANSI_OPERATOR "-="); break;
		case sno_TK_ASSIGNMUL: printf(ANSI_OPERATOR "*="); break;
		case sno_TK_ASSIGNDIV: printf(ANSI_OPERATOR "/="); break;
		case sno_TK_ASSIGNIDIV: printf(ANSI_OPERATOR "//="); break;
		case sno_TK_ASSIGNPOW: printf(ANSI_OPERATOR "**="); break;
		case sno_TK_ASSIGNMOD: printf(ANSI_OPERATOR "%%="); break;
		case sno_TK_ASSIGNBAND: printf(ANSI_OPERATOR "&="); break;
		case sno_TK_ASSIGNBOR: printf(ANSI_OPERATOR "|="); break;
		case sno_TK_ASSIGNBXOR: printf(ANSI_OPERATOR "^="); break;
		case sno_TK_ASSIGNSHL: printf(ANSI_OPERATOR "<<="); break;
		case sno_TK_ASSIGNSHR: printf(ANSI_OPERATOR ">>="); break;
		case sno_TK_INC: printf(ANSI_OPERATOR "++"); break;
		case sno_TK_DEC: printf(ANSI_OPERATOR "--"); break;
		case sno_TK_BITFLIP: printf(ANSI_OPERATOR "~"); break;
		case sno_TK_LNOT: printf(ANSI_OPERATOR "!"); break;
		case sno_TK_LAND: printf(ANSI_OPERATOR "&&"); break;
		case sno_TK_LOR: printf(ANSI_OPERATOR "||"); break;
		case sno_TK_EQ: printf(ANSI_OPERATOR "=="); break;
		case sno_TK_NEQ: printf(ANSI_OPERATOR "!="); break;
		case sno_TK_LT: printf(ANSI_OPERATOR "<"); break;
		case sno_TK_GT: printf(ANSI_OPERATOR ">"); break;
		case sno_TK_LE: printf(ANSI_OPERATOR "<="); break;
		case sno_TK_GE: printf(ANSI_OPERATOR ">="); break;
		case sno_TK_LPAREN: printf(ANSI_SEPARATOR "("); break;
		case sno_TK_RPAREN: printf(ANSI_SEPARATOR ")"); break;
		case sno_TK_LBRACKET: printf(ANSI_SEPARATOR "["); break;
		case sno_TK_RBRACKET: printf(ANSI_SEPARATOR "]"); break;
		case sno_TK_LBRACE: printf(ANSI_SEPARATOR "{"); break;
		case sno_TK_RBRACE: printf(ANSI_SEPARATOR "}"); break;
		case sno_TK_DOT: printf(ANSI_OPERATOR "."); break;
		case sno_TK_COMMA: printf(ANSI_SEPARATOR ","); break;
		case sno_TK_COLON: printf(ANSI_SEPARATOR ":"); break;
		case sno_TK_NUMBER: printf(ANSI_NUMBER "%g", (double)token->info.u_number); break;
		case sno_TK_STRING: printf(
			ANSI_STRING "\"%.*s\"",
			(unsigned int)token->info.u_string->length,
			sno_string_chars(token->info.u_string)
		); break;
		case sno_TK_INTERPOLATED_STRING: printf(
			ANSI_STRING "\"%.*s\" " ANSI_CONST "\\()",
			(unsigned int)token->info.u_string->length,
			sno_string_chars(token->info.u_string)
		); break;
		case sno_TK_IDENTIFIER:
		{
			/*const char* const str = sno_string_chars(token->info.u_string);
			const size_t len = token->info.u_string->length;
			if (next_token && next_token->type == sno_TK_LPAREN) {
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
				token->info.u_string->length,
				sno_string_chars(token->info.u_string)
			);
			break;
		}
		default: sno_unreachable; break;
		}
	}
	printf(ANSI_NORMAL);
}



static sno_inline sno_Bool check_next(sno_Tokenizer* ts, char c) {
	if (*ts->cur_char == c) {
		ts->cur_char++;
		return sno_TRUE;
	} else {
		return sno_FALSE;
	}
}

static sno_inline sno_Bool check_next_alphanumeric(sno_Tokenizer* ts) {
	char c = *ts->cur_char;
	if (is_alpha(c) || is_digit(c)) {
		ts->cur_char++;
		return sno_TRUE;
	} else {
		return sno_FALSE;
	}
}




static sno_inline sno_Number string_to_number(const char* string, size_t length) {
	sno_assert(length < 256);
	char zero_terminated[256];
	memcpy(zero_terminated, string, length);
	zero_terminated[length] = '\0';
	return strtod(zero_terminated, NULL);
}

// Read number token which will be either int or float. Not hex or binary
static void read_normal_number(sno_Tokenizer* ts, sno_Token* token) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);

	sno_Bool has_decimal = sno_FALSE;
	while (1) {
		ts->cur_char++;
		if (is_digit(*ts->cur_char)) {
			continue;
		} else if (*ts->cur_char == '.') {
			if (has_decimal) {
				sno_throw_syntax_error_at(
					ts,
					ts->token_start - sno_string_chars(ts->source_code),
					"There are multiple decimal points in this number"
				);
			}
			has_decimal = sno_TRUE;
			continue;
		} else if (is_alpha(*ts->cur_char)) {
			sno_throw_syntax_error_at(
				ts,
				ts->cur_char - sno_string_chars(ts->source_code),
				"There is a letter character in this number"
			);
		} else {
			break;
		}
	}

	uint32_t length = (uint32_t)(ts->cur_char - ts->token_start);
	if (length >= 255) {
		sno_throw_syntax_error_at(
			ts,
			ts->token_start - sno_string_chars(ts->source_code),
			"This number is way too long"
		);
	}
	token->info.u_number = string_to_number(ts->token_start, length);
}



static void read_string_literal(sno_Tokenizer* ts, sno_Token* token, sno_Bool* interpolated) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);
	sno_assert_ptr(interpolated);

	sno_State* state = ts->main_state;
	sno_DynArray formatted_string;
	sno_dynarray_init(state, &formatted_string, 1, 512);
	const char* start = ts->cur_char;
	while (1) {
		ts->cur_char++;
		switch (*ts->cur_char) {
		case '\n': case '\r': case '\0': {
			sno_throw_syntax_error_at(
				ts,
				ts->token_start - sno_string_chars(ts->source_code),
				"This string is missing closing quotes"
			);
			break;
		}
		case '\t': {
			sno_throw_syntax_error_at(
				ts,
				ts->cur_char - sno_string_chars(ts->source_code),
				"Strings cannot contain tabs"
			);
			break;
		}
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
			case '\'': escapedchar = '\''; break;
			case '0':  escapedchar = '\0'; break;
			case '(': {
				// Interpolated string
				ts->string_interpolation_depth++;
				*interpolated = sno_TRUE;
				goto endstring;
			}
			default:
				sno_throw_syntax_error_at(
					ts,
					ts->cur_char - sno_string_chars(ts->source_code) - 1,
					"Invalid string escape character '\\%c'",
					*ts->cur_char
				);
				break;
			}
			sno_dynarray_push_back(state, &formatted_string, 1, &escapedchar);
			break;
		}
		case '\'': case '\"': {
			goto endstring;
		}

		default:
			sno_dynarray_push_back(state, &formatted_string, 1, ts->cur_char);
			break;
		}
	}
endstring:
	ts->cur_char++; // Skip the closing double quotes
	token->info.u_string = sno_create_string(
		state,
		(const char*)formatted_string.buffer,
		formatted_string.count
	);
	sno_dynarray_clear(state, &formatted_string, 1);
}



static void read_comment(sno_Tokenizer* ts) {
	sno_assert_ptr(ts);
	for (;;) {
		ts->cur_char++;
		if (*ts->cur_char == '\n' ||
			*ts->cur_char == '\r' ||
			*ts->cur_char == '\0') break;
	}
}

static void read_multiline_comment(sno_Tokenizer* ts) {
	sno_assert_ptr(ts);
	const char* start = ts->cur_char;
	for (;;) {
		sno_assert(ts->cur_char <= ts->source_code_end);
		if (ts->cur_char == ts->source_code_end) {
			sno_throw_syntax_error_at(
				ts,
				start - 2 - sno_string_chars(ts->source_code),
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
	sno_Tokenizer* ts,
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
			sno_throw_syntax_error_at(
				ts,
				backslash - sno_string_chars(ts->source_code),
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
			} else if (next == '*') {
				ts->cur_char += 2;
				read_multiline_comment(ts);
			} else {
				return stmt_end;
			}
			break;
		}
		default: {
			return stmt_end;
		}
		}
		ts->cur_char++;
	}
	return sno_TRUE;
}

static sno_TokenType lex_token(sno_Tokenizer* ts, sno_Token* token) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);

	ts->token_start = ts->cur_char;
	token->source_code_pos = ts->token_start - sno_string_chars(ts->source_code);

	sno_assert(ts->cur_char <= ts->source_code_end);
	if (ts->cur_char == ts->source_code_end) {
		return sno_TK_EOF;
	}

	switch (*ts->cur_char) {
	case '+': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNADD;
		else if (check_next(ts, '+')) return sno_TK_INC;
		else return sno_TK_ADD;
	}
	case '-': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNSUB;
		else if (check_next(ts, '-')) return sno_TK_DEC;
		else return sno_TK_SUB;
	}
	case '*': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNMUL;
		else if (check_next(ts, '*')) {
			if (check_next(ts, '=')) return sno_TK_ASSIGNPOW;
			else return sno_TK_POW;
		} else return sno_TK_MUL;
	}
	case '/': {
		ts->cur_char++;
		sno_assert(!check_next(ts, '/'));
		sno_assert(!check_next(ts, '*'));
		if (check_next(ts, '=')) return sno_TK_ASSIGNDIV;
		else if (check_next(ts, '-')) {
			if (check_next(ts, '=')) return sno_TK_ASSIGNIDIV;
			else return sno_TK_IDIV;
		} else return sno_TK_DIV;
	}
	case '%': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNMOD;
		else return sno_TK_MOD;
	}
	case '&': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNBAND;
		else if (check_next(ts, '&')) return sno_TK_LAND;
		else return sno_TK_BAND;
	}
	case '|': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNBOR;
		else if (check_next(ts, '|')) return sno_TK_LOR;
		else return sno_TK_BOR;
	}
	case '^': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_ASSIGNBXOR;
		else return sno_TK_BXOR;
	}
	case '<': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_LE;
		else if (check_next(ts, '<')) {
			if (check_next(ts, '=')) return sno_TK_ASSIGNSHL;
			else return sno_TK_SHL;
		} else return sno_TK_LT;
	}
	case '>': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_GE;
		else if (check_next(ts, '>')) {
			if (check_next(ts, '=')) return sno_TK_ASSIGNSHR;
			else return sno_TK_SHR;
		} else return sno_TK_GT;
	}
	case '=': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_EQ;
		else return sno_TK_ASSIGN;
	}
	case '~': {
		ts->cur_char++;
		return sno_TK_BITFLIP;
	}
	case '!': {
		ts->cur_char++;
		if (check_next(ts, '=')) return sno_TK_NEQ;
		else return sno_TK_LNOT;
	}
	case '(': {
		ts->cur_char++;
		return sno_TK_LPAREN;
	}
	case ')': {
		ts->cur_char++;
		return sno_TK_RPAREN;
	}
	case '[': {
		ts->cur_char++;
		return sno_TK_LBRACKET;
	}
	case ']': {
		ts->cur_char++;
		return sno_TK_RBRACKET;
	}
	case '{': {
		ts->cur_char++;
		return sno_TK_LBRACE;
	}
	case '}': {
		ts->cur_char++;
		return sno_TK_RBRACE;
	}
	case '.': {
		ts->cur_char++;
		return sno_TK_DOT;
	}
	case ',': {
		ts->cur_char++;
		return sno_TK_COMMA;
	}
	case ':': {
		ts->cur_char++;
		return sno_TK_COLON;
	}
	case '\'': case '\"': {
		sno_Bool interpolated = sno_FALSE;
		read_string_literal(ts, token, &interpolated);
		return sno_TK_STRING + interpolated;
	}

	case '0': {
		if (check_next(ts, 'x')) {
			// Hex number
			sno_not_implemented;
		}
		if (check_next(ts, 'b')) {
			// Binary number
			sno_not_implemented;
		}
		// Fall through
	}
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9': {
		read_normal_number(ts, token);
		return sno_TK_NUMBER;
	}

	case 'b': {
		ts->cur_char++;
		if (!check_next(ts, 'r')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (!check_next(ts, 'a')) goto identifier;
		if (!check_next(ts, 'k')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return sno_TK_BREAK;
	}
	case 'c': {
		ts->cur_char++;
		if (!check_next(ts, 'o')) goto identifier;
		if (!check_next(ts, 'n')) goto identifier;
		if (check_next(ts, 's')) {
			if (!check_next(ts, 't')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_CONST;
		} else if (check_next(ts, 't')) {
			if (!check_next(ts, 'i')) goto identifier;
			if (!check_next(ts, 'n')) goto identifier;
			if (!check_next(ts, 'u')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_CONTINUE;
		}
		goto identifier;
	}
	case 'e': {
		ts->cur_char++;
		if (!check_next(ts, 'l')) goto identifier;
		if (!check_next(ts, 's')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return sno_TK_ELSE;
	}
	case 'f': {
		ts->cur_char++;
		if (check_next(ts, 'a')) {
			if (!check_next(ts, 'l')) goto identifier;
			if (!check_next(ts, 's')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_FALSE;
		} else if (check_next(ts, 'o')) {
			if (!check_next(ts, 'r')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_FOR;
		} else if (check_next(ts, 'u')) {
			if (!check_next(ts, 'n')) goto identifier;
			if (!check_next(ts, 'c')) goto identifier;
			if (!check_next(ts, 't')) goto identifier;
			if (!check_next(ts, 'i')) goto identifier;
			if (!check_next(ts, 'o')) goto identifier;
			if (!check_next(ts, 'n')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_FUNCTION;
		}
		goto identifier;
	}
	case 'i': {
		ts->cur_char++;
		if (check_next(ts, 'f')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_IF;
		} else if (check_next(ts, 'n')) {
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_IN;
		}
		goto identifier;
	}
	case 'n': {
		ts->cur_char++;
		if (!check_next(ts, 'o')) goto identifier;
		if (!check_next(ts, 'n')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return sno_TK_NONE;
	}
	case 'r': {
		ts->cur_char++;
		if (!check_next(ts, 'e')) goto identifier;
		if (!check_next(ts, 't')) goto identifier;
		if (!check_next(ts, 'u')) goto identifier;
		if (!check_next(ts, 'r')) goto identifier;
		if (!check_next(ts, 'n')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return sno_TK_RETURN;
	}
	case 's': {
		ts->cur_char++;
		if (!check_next(ts, 'e')) goto identifier;
		if (!check_next(ts, 'l')) goto identifier;
		if (!check_next(ts, 'f')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return sno_TK_SELF;
	}
	case 't': {
		ts->cur_char++;
		if (!check_next(ts, 'r')) goto identifier;
		if (!check_next(ts, 'u')) goto identifier;
		if (!check_next(ts, 'e')) goto identifier;
		if (check_next_alphanumeric(ts)) goto identifier;
		return sno_TK_TRUE;
	}
	case 'v': {
		ts->cur_char++;
		if (check_next(ts, 'a')) {
			if (!check_next(ts, 'r')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_VAR;
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
		return sno_TK_WHILE;
	}

	case 'a':                     case 'd':
	case 'g': case 'h': case 'j': case 'k': case 'l':
	case 'm':           case 'o': case 'p': case 'q':
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
		token->info.u_string = sno_create_string(
			ts->main_state,
			ts->token_start,
			length
		);
		return sno_TK_IDENTIFIER;
	}

	default: {
		// All other characters are invalid
		sno_throw_syntax_error_at(
			ts,
			ts->token_start - sno_string_chars(ts->source_code),
			"There is an invalid character here"
		);
	}
	}
	sno_unreachable;
	return sno_TK_ERROR;
}



void sno_read_initial_token(sno_Tokenizer* ts) {
	(void)skip_whitespace_and_comments(ts, sno_FALSE);
	ts->token.type = sno_TK_EOF; // Will become prev_token
	ts->insert_terminator = sno_FALSE;
	sno_read_next_token(ts);
}

void sno_read_next_token(sno_Tokenizer* ts) {
	sno_assert_ptr(ts);
	ts->prev_token = ts->token;
	if (ts->insert_terminator) {
		ts->insert_terminator = sno_FALSE;
		ts->token.type = sno_TK_TERMINATOR;
	} else {
		ts->token.type = lex_token(ts, &ts->token);
		sno_Bool insert_terminator_on_endline = sno_FALSE;
		switch (ts->token.type) {
		case sno_TK_IDENTIFIER:
		case sno_TK_NUMBER:
		case sno_TK_STRING:
		case sno_TK_BREAK:
		case sno_TK_CONTINUE:
		case sno_TK_RETURN:
		case sno_TK_RPAREN:
		case sno_TK_RBRACKET:
		case sno_TK_RBRACE:
			insert_terminator_on_endline = sno_TRUE;
		default: break;
		}
		ts->insert_terminator = skip_whitespace_and_comments(
			ts,
			insert_terminator_on_endline
		);
	}
}

void sno_continue_interpolated_string(sno_Tokenizer* ts) {
	sno_assert_ptr(ts);
	sno_Bool interpolated = sno_FALSE;
	read_string_literal(ts, &ts->token, &interpolated);
	ts->token.type = sno_TK_STRING + interpolated;
	sno_assert(!ts->insert_terminator);
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

static size_t find_column_number(const char* line_start, const char* view) {
	return 69420;
}

static int sprint_line(
	char* buffer,
	int buffer_size,
	const sno_String* source_code,
	uint32_t pos,
	uint32_t extra_pos,
	uint32_t* out_spaces_before_pos,
	sno_Bool* out_extra_pos_is_on_same_line
) {
	const char* string = sno_string_chars(source_code);
	size_t string_length = source_code->length;
	const char* string_end = string + string_length;
	sno_assert(pos <= string_length);
	const char* string_pos = string + pos;

	const char* line_start = find_line_start(string, string_pos);
	size_t line = find_line_number(string, line_start);
	size_t column = find_column_number(line_start, string_pos);
	const char* p = find_first_non_whitespace_char_on_line(line_start, string_end);

	int length = 0;
	size_t spaces_before_pos = 0;
	length += snprintf(buffer + length, buffer_size - length, sno_ANSI_CYAN "        |  \n");
	length += snprintf(buffer + length, buffer_size - length, " %5u  |  " sno_ANSI_NORMAL, (unsigned int)line);
	while (p < string_pos) {
		if (*p == '\t') {
			spaces_before_pos &= ~(7);
			spaces_before_pos += 4;
			length &= ~(7);
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
		if (p == extra_pos) {
			*out_extra_pos_is_on_same_line = sno_TRUE;
		}
		buffer[length++] = *(p++);
	}
	length += snprintf(buffer + length, buffer_size - length, "\n" sno_ANSI_CYAN "        |  " sno_ANSI_RED);
	for (size_t i = 0; i < spaces_before_pos; i++) {
		buffer[length++] = ' ';
	}
	*out_spaces_before_pos = spaces_before_pos;
	return length;
}

static int sprint_and_underline_pos_on_line(
	char* buffer,
	int buffer_size,
	const sno_String* source_code,
	uint32_t pos,
	uint32_t underline_length
) {
	uint32_t spaces_before_pos;
	sno_Bool unused2;
	int length = sprint_line(
		buffer,
		buffer_size,
		source_code,
		pos,
		0,
		&spaces_before_pos,
		&unused2
	);
	for (size_t i = 0; i < underline_length; i++) {
		if (length >= buffer_size - 1) break;
		buffer[length++] = '~';
	}
	buffer[length++] = ' ';
	sno_assert(length <= buffer_size);
	return length;
}



int sno_sprintf_source_code_pos(
	sno_State* state,
	char* buffer,
	int buffer_size,
	const sno_String* source_code,
	uint32_t source_code_pos
) {
	int length = sprint_and_underline_pos_on_line(
		buffer,
		buffer_size,
		source_code,
		source_code_pos,
		1
	);
	return length;
}



sno_no_return void sno_throw_syntax_error_at(
	sno_Tokenizer* ts,
	uint32_t pos,
	const char* format,
	...
) {
	va_list args;
	va_start(args, format);
	sno_throw_at_source_code_pos(
		ts->main_state,
		sno_EXCEPTION_SYNTAX_ERROR,
		ts->source_code,
		ts->source_code_name,
		pos,
		format,
		args
	);
	va_end(args);
}

sno_no_return void sno_throw_syntax_error_at_cur_token(
	sno_Tokenizer* ts,
	const char* format,
	...
) {
	va_list args;
	va_start(args, format);
	sno_throw_at_source_code_pos(
		ts->main_state,
		sno_EXCEPTION_SYNTAX_ERROR,
		ts->source_code,
		ts->source_code_name,
		ts->token.source_code_pos,
		format,
		args
	);
	//vprintf(format, args);
	va_end(args);
	//sno_throw(ts->main_state, sno_EXCEPTION_SYNTAX_ERROR, "why", 3);
}

sno_no_return void sno_throw_syntax_error_open_close(
	sno_Tokenizer* ts,
	uint32_t pos_open,
	uint32_t pos_close,
	const char* format,
	...
) {
	va_list args;
	va_start(args, format);
	sno_throw_at_source_code_pos_open_close(
		ts->main_state,
		sno_EXCEPTION_SYNTAX_ERROR,
		ts->source_code,
		ts->source_code_name,
		pos_open,
		pos_close,
		format,
		args
	);
	va_end(args);
}





static void print_source_code_throws(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
) {
	sno_Tokenizer ts = { 0 };
	ts.main_state = state;
	ts.source_code_name = name;
	ts.source_code = source_code;
	ts.source_code_end = sno_string_chars(source_code) + source_code->length;
	ts.cur_char = sno_string_chars(source_code);
	ts.token_start = sno_string_chars(source_code);
	ts.cs = NULL;

	sno_read_initial_token(&ts);

	sno_Bool new_stmt = sno_TRUE;
	while (1) {
		if (new_stmt) {
			printf("stmt | ");
		}
		sno_print_token(&ts.token);
		if (ts.token.type == sno_TK_TERMINATOR) {
			putchar('\n');
		} else {
			putchar(' ');
		}
		new_stmt = ts.token.type == sno_TK_TERMINATOR;
		
		sno_read_next_token(&ts);
		if (ts.token.type < 0) {
			break;
		}
	}
	putchar('\n');
	putchar('\n');
}

sno_Bool sno_print_source_code(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
) {
	sno_assert_ptr(state);
	sno_assert_ptr(source_code);
	sno_assert(source_code->length <= sno_SIZE_T_LIMIT);

	sno_Bool success = sno_TRUE;
	sno_ExceptionJump exception_jump;
	exception_jump.prev = state->exception_jump;
	state->exception_jump = &exception_jump;
	if (setjmp(exception_jump.buf) == 0) {
		print_source_code_throws(state, name, source_code);
	} else {
		fprintf(
			stderr,
			"\n%.*s\n",
			(unsigned int)state->exception_msg->length,
			sno_string_chars(state->exception_msg)
		);
		success = sno_FALSE;
	}
	state->exception_jump = state->exception_jump->prev;
	return success;
}
