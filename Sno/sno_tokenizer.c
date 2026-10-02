#include "sno_compiler.h"

#include <stdio.h>
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
	printf(ANSI_NORMAL);
}



static sno_inline sno_Bool check_next(Tokenizer* ts, char c) {
	if (*ts->cur_char == c) {
		ts->cur_char++;
		return sno_TRUE;
	} else {
		return sno_FALSE;
	}
}





static void read_comment(Tokenizer* ts) {
	sno_assert_ptr(ts);
	while (ts->cur_char < ts->source_code_end) {
		ts->cur_char++;
		if (*ts->cur_char == '\n' ||
			*ts->cur_char == '\r' ||
			*ts->cur_char == '\0') break;
	}
}

static void read_multiline_comment(Tokenizer* ts) {
	sno_assert_ptr(ts);
	const char* start = ts->cur_char;
	while (1) {
		sno_assert(ts->cur_char <= ts->source_code_end);
		if (ts->cur_char == ts->source_code_end) {
			throw_syntax_error(
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
			throw_syntax_error(
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



void read_first_token(Tokenizer* ts) {
	sno_assert_ptr(ts);

}

void read_next_token(Tokenizer* ts) {
	sno_assert_ptr(ts);

}



static void print_source_code_throws(
	sno_VMState* vm,
	const IString* source_code_name,
	const IString* source_code
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
	const IString* source_code_name,
	const IString* source_code
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
		fprintf(stderr, sno_ANSI_RED "Source code throws" sno_ANSI_NORMAL);
		success = sno_FALSE;
	}
	vm->exception_jump = vm->exception_jump->prev;
	return success;
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

size_t sprint_source_code_context(
	char* buffer,
	size_t buffer_size,
	const IString* source_code,
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
	return length;
}



sno_no_return void throw_syntax_error(
	Tokenizer* ts,
	uint32_t pos,
	const char* const message_format,
	...
) {
	sno_assert_ptr(ts);
	sno_assert_ptr(pos < ts->source_code->length);

	char buffer[sno_STACK_BUFFER_LENGTH];
	size_t length = 0;
	length += sprint_source_code_context(
		buffer,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		ts->source_code,
		pos
	);
	va_list args;
	va_start(args, message_format);
	length += (size_t)vsnprintf(
		buffer,
		sno_STACK_BUFFER_LENGTH - 1 - length,
		message_format,
		args
	);
	va_end(args);
	vm_throw(ts->parent_vm, EXCEPTION_SYNTAX_ERROR, buffer, length);
}
