#include "sno_parser.h"

#include "sno_state.h"
#include <string.h>

#define is_alpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && c <= 'Z') || (c) == '_')
#define is_digit(c) ((c) >= '0' && (c) <= '9')

const char* const sno_token_strings[sno_NUM_TOKENS] = {
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
	"identifier",
};

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
static sno_TokenType read_normal_number(sno_Tokenizer* ts, sno_Token* token) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);

	sno_Bool has_decimal = sno_FALSE;
	while (1) {
		ts->cur_char++;
		if (is_digit(*ts->cur_char)) {
			continue;
		} else if (*ts->cur_char == '.') {
			if (has_decimal) {
				//throw_syntax_error(ts, "Two decimal points in one number");
			}
			has_decimal = sno_TRUE;
			continue;
		} else if (is_alpha(*ts->cur_char)) {
			//throw_syntax_error(ts, "Alpha token directly after number");
		} else {
			break;
		}
	}

	uint32_t length = (uint32_t)(ts->cur_char - ts->token_start);
	if (length >= 255) {
		//throw_syntax_error(ts, "Number token is too long");
	}
	
	token->info.u_number = string_to_number(ts->token_start, length);
	return sno_TK_NUMBER;
}



static sno_TokenType read_string_literal(sno_Tokenizer* ts, sno_Token* token) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);

	sno_State* state = ts->main_state;
	sno_DynArray formatted_string;
	sno_dynarray_init(state, &formatted_string, 1, 512);
	for (;;) {
		ts->cur_char++;
		switch (*ts->cur_char) {
		case '\n': case '\r': case '\0':
		{
			//throw_syntax_error(ts, "String is missing closing double quotes");
			break;
		}
		case '\\':
		{
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
			case '{':  escapedchar = '{';  break;
			case '0':  escapedchar = '\0'; break;
			default:
				//throw_syntax_error(ts, "Invalid escape character");
				break;
			}
			sno_dynarray_push_back(state, &formatted_string, 1, &escapedchar);
			break;
		}
		case '\'': case '\"':
		{
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
		(char*)formatted_string.buffer,
		formatted_string.count
	);
	sno_dynarray_clear(state, &formatted_string);
	return sno_TK_STRING;
}



static void read_comment(sno_Tokenizer* ts) {
	sno_assert_ptr(ts);
	for (;;) {
		ts->cur_char++;
		if (*ts->cur_char == '\n' ||
			*ts->cur_char == '\r' ||
			*ts->cur_char == '\0') break;
	}
	ts->token_start = ts->cur_char;
}

static void read_multiline_comment(sno_Tokenizer* ts) {
	sno_assert_ptr(ts);
	for (;;) {
		if (*ts->cur_char == '\0') {
			//throw_syntax_error(ts, "Multi-line comment doesn't end");
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
		if (*ts->cur_char == '\n') {
			ts->line++;
		}
		ts->cur_char++;
	}
	ts->token_start = ts->cur_char;
}



static sno_TokenType lex_token(sno_Tokenizer* ts, sno_Token* token, sno_Bool* stmt_end) {
	sno_assert_ptr(ts);
	sno_assert_ptr(token);
	sno_assert_ptr(stmt_end);

	ts->token_start = ts->cur_char;

	while (1) {
		char c = *ts->cur_char;
		switch (c) {
		case '\0':
		{
			*stmt_end = sno_TRUE;
			return sno_TK_EOF;
		}
		case '\n':
		{
			*stmt_end = sno_TRUE;
			ts->line++;
			ts->token_start++;
			ts->cur_char++;
			break;
		}
		case '\\':
		{
			ts->cur_char++;
			if (check_next(ts, '\n') || check_next(ts, '\r') && check_next(ts, '\n')) {
				ts->line++;
				ts->token_start++;
				break;
			}
			//throw_syntax_error(ts, "Invalid endline backslash character");
		}
		case ' ': case '\f': case '\t': case '\v': case '\r':
		{
			ts->token_start++;
			ts->cur_char++;
			break;
		}
		case ';':
		{
			*stmt_end = sno_TRUE;
			ts->token_start++;
			ts->cur_char++;
			break;
		}

		case '+':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNADD;
			else if (check_next(ts, '+')) return sno_TK_INC;
			else return sno_TK_ADD;
		}
		case '-':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNSUB;
			else if (check_next(ts, '-')) return sno_TK_DEC;
			else return sno_TK_SUB;
		}
		case '*':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNMUL;
			else if (check_next(ts, '*')) {
				if (check_next(ts, '=')) return sno_TK_ASSIGNPOW;
				else return sno_TK_POW;
			} else return sno_TK_MUL;
		}
		case '/':
		{
			ts->cur_char++;
			if (check_next(ts, '/')) {
				read_comment(ts);
				break;
			}
			if (check_next(ts, '*')) {
				read_multiline_comment(ts);
				break;
			}
			if (check_next(ts, '=')) return sno_TK_ASSIGNDIV;
			else if (check_next(ts, '-')) {
				if (check_next(ts, '=')) return sno_TK_ASSIGNIDIV;
				else return sno_TK_IDIV;
			} else return sno_TK_DIV;
		}
		case '%':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNMOD;
			else return sno_TK_MOD;
		}
		case '&':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNBAND;
			else if (check_next(ts, '&')) return sno_TK_LAND;
			else return sno_TK_BAND;
		}
		case '|':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNBOR;
			else if (check_next(ts, '|')) return sno_TK_LOR;
			else return sno_TK_BOR;
		}
		case '^':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_ASSIGNBXOR;
			else return sno_TK_BXOR;
		}
		case '<':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_LE;
			else if (check_next(ts, '<')) {
				if (check_next(ts, '=')) return sno_TK_ASSIGNSHL;
				else return sno_TK_SHL;
			} else return sno_TK_LT;
		}
		case '>':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_GE;
			else if (check_next(ts, '>')) {
				if (check_next(ts, '=')) return sno_TK_ASSIGNSHR;
				else return sno_TK_SHR;
			} else return sno_TK_GT;
		}
		case '=':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_EQ;
			else return sno_TK_ASSIGN;
		}
		case '~':
		{
			ts->cur_char++;
			return sno_TK_BITFLIP;
		}
		case '!':
		{
			ts->cur_char++;
			if (check_next(ts, '=')) return sno_TK_NEQ;
			else return sno_TK_LNOT;
		}
		case '(':
		{
			ts->cur_char++;
			return sno_TK_LPAREN;
		}
		case ')':
		{
			ts->cur_char++;
			return sno_TK_RPAREN;
		}
		case '[':
		{
			ts->cur_char++;
			return sno_TK_LBRACKET;
		}
		case ']':
		{
			ts->cur_char++;
			return sno_TK_RBRACKET;
		}
		case '{':
		{
			ts->cur_char++;
			return sno_TK_LBRACE;
		}
		case '}':
		{
			ts->cur_char++;
			return sno_TK_RBRACE;
		}
		case '.':
		{
			ts->cur_char++;
			return sno_TK_DOT;
		}
		case ',':
		{
			ts->cur_char++;
			return sno_TK_COMMA;
		}
		case ':':
		{
			ts->cur_char++;
			return sno_TK_COLON;
		}
		case '\'': case '\"':
		{
			return read_string_literal(ts, token);
		}

		case '0':
		{
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
		case '5': case '6': case '7': case '8': case '9':
		{
			return read_normal_number(ts, token);
		}

		case 'b':
		{
			ts->cur_char++;
			if (!check_next(ts, 'r')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (!check_next(ts, 'a')) goto identifier;
			if (!check_next(ts, 'k')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_BREAK;
		}
		case 'c':
		{
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
		case 'e':
		{
			ts->cur_char++;
			if (!check_next(ts, 'l')) goto identifier;
			if (!check_next(ts, 's')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_ELSE;
		}
		case 'f':
		{
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
		case 'i':
		{
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
		case 'n':
		{
			ts->cur_char++;
			if (!check_next(ts, 'o')) goto identifier;
			if (!check_next(ts, 'n')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_NONE;
		}
		case 'r':
		{
			ts->cur_char++;
			if (!check_next(ts, 'e')) goto identifier;
			if (!check_next(ts, 't')) goto identifier;
			if (!check_next(ts, 'u')) goto identifier;
			if (!check_next(ts, 'r')) goto identifier;
			if (!check_next(ts, 'n')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_RETURN;
		}
		case 's':
		{
			ts->cur_char++;
			if (!check_next(ts, 'e')) goto identifier;
			if (!check_next(ts, 'l')) goto identifier;
			if (!check_next(ts, 'f')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_SELF;
		}
		case 't':
		{
			ts->cur_char++;
			if (!check_next(ts, 'r')) goto identifier;
			if (!check_next(ts, 'u')) goto identifier;
			if (!check_next(ts, 'e')) goto identifier;
			if (check_next_alphanumeric(ts)) goto identifier;
			return sno_TK_TRUE;
		}
		case 'v':
		{
			ts->cur_char++;
			if (check_next(ts, 'a')) {
				if (!check_next(ts, 'r')) goto identifier;
				if (check_next_alphanumeric(ts)) goto identifier;
				return sno_TK_VAR;
			}
			goto identifier;
		}
		case 'w':
		{
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
			size_t len = ts->cur_char - ts->token_start;
			token->info.u_string = sno_create_string(
				ts->main_state,
				ts->token_start,
				len
			);
			return sno_TK_IDENTIFIER;
		}

		default:
		{
			/* All other characters are invalid */
			//throw_syntax_error(ts, "Invalid character");
		}
		}
	}
	sno_unreachable;
}



void sno_init_tokenizer(sno_Tokenizer* ts) {
	sno_Bool unused;
	ts->cur_token.type = lex_token(ts, &ts->cur_token, &unused);
	if (ts->cur_token.type < 0) {
		return;
	}
	ts->cur_token.stmt_end = sno_FALSE;
	ts->next_token.type = lex_token(ts, &ts->next_token, &ts->cur_token.stmt_end);
}

void sno_throw_syntax_error(sno_State* state, sno_LineNumber line, sno_ColumnNumber column) {

}

void sno_read_next_token(sno_Tokenizer* ts) {

}



static void print_source_code_throws(sno_State* state, const char* const string, size_t length) {
	sno_Tokenizer ts = { 0 };
	ts.main_state = state;
	ts.source_code_string = string;
	ts.source_code_length = length;
	ts.cur_char = string;
	ts.token_start = string;
	ts.line = 1;
	ts.column = 1;
	ts.cs = NULL;

	sno_init_tokenizer(&ts);

	/*sno_Bool new_stmt = sno_TRUE;
	while (1) {
		if (new_stmt) {
			printf("stmt on line % 5i | ", ts.cur_token.line);
		}
		sno_PrintToken(&ts.cur_token, &ts.next_token, sno_TRUE);
		putchar(' ');
		new_stmt = ts.cur_token.stmt_end;
		sno_ReadNextToken(&ts);
		if (ts.cur_token.type < 0) {
			break;
		}
		if (new_stmt) {
			putchar('\n');
		}
	}*/
	putchar('\n');
	putchar('\n');
}

sno_Bool sno_print_source_code(sno_State* state, const char* const string, size_t length) {
	sno_assert_ptr(state);
	sno_assert_ptr(string);
	sno_assert(length <= sno_SIZE_T_LIMIT);



	return sno_TRUE;
}
