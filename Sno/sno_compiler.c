#include "sno_compiler.h"

#define DEBUG_PRINT_PARSER

DEFINE_GENERIC_DYN_ARRAY(CompilerInstruction, Instruction, instruction);
DEFINE_GENERIC_DYN_ARRAY(Bytecode, Bytecode, bytecode);



static sno_no_return void syntax_error_at_cur_token(
	Tokenizer* ts,
	const char* const format,
	...
) {
	sno_assert_ptr(ts);
	sno_assert_ptr(format);
	va_list args;
	va_start(args, format);
	vsyntax_error(ts, ts->token.pos, format, args);
}



static Bytecode* create_bytecode(sno_VMState* vm) {
	Bytecode* bytecode = state_alloc(vm->state, sizeof(Bytecode));
	memset(bytecode, 0, sizeof(Bytecode));
	return bytecode;
}

static void init_function_compiler(Tokenizer* ts, Compiler* cs) {
	sno_assert(ts);
	//sno_VMState* vm = ts->parent_vm;
	instruction_dyn_array_init(&cs->instructions);
	number_dyn_array_init(&cs->number_constants);
	istring_dyn_array_init(&cs->string_constants);
	bytecode_dyn_array_init(&cs->bytecode_constants);

	cs->ts = ts;
}

static Bytecode* free_function_compiler(Tokenizer* ts, Compiler* cs) {
	Bytecode* bytecode = create_bytecode(ts->parent_vm);
	
	ts->cs = cs->parent_function;
	return bytecode;
}



static ConstID add_number_constant(Compiler* cs, sno_Number number) {
	sno_assert_ptr(cs);
	sno_VMState* vm = cs->ts->parent_vm;
	if (cs->number_constants.count > MAX_NUMBER_CONSTANTS) {
		syntax_error_at_cur_token(
			cs->ts,
			"There are too many numbers in this function"
		);
	}
	for (size_t i = 0; i < cs->number_constants.count; i++) {
		if (number == cs->number_constants.buffer[i]) {
			return (ConstID)i;
		}
	}
	// Add new number constant
	number_dyn_array_push(vm, &cs->number_constants, &number);
	return (ConstID)(cs->number_constants.count - 1);
}

static ConstID add_string_constant(Compiler* cs, const IString* string) {
	sno_assert_ptr(cs);
	sno_assert_ptr(string);
	sno_VMState* vm = cs->ts->parent_vm;
	if (cs->string_constants.count > MAX_STRING_CONSTANTS) {
		syntax_error_at_cur_token(
			cs->ts,
			"There are too many strings in this function"
		);
	}
	for (size_t i = 0; i < cs->string_constants.count; i++) {
		if (string == cs->string_constants.buffer[i]) {
			return (ConstID)i;
		}
	}
	// Add new number constant
	istring_dyn_array_push(vm, &cs->string_constants, &string);
	return (ConstID)(cs->string_constants.count - 1);
}



static size_t emit(
	Tokenizer* ts,
	OpCode opcode,
	uint16_t arg,
	SourceCodePos pos
) {
	sno_assert_ptr(ts);
	CompilerInstruction instruction;
	instruction.d.opcode = opcode;
	instruction.d.arg = arg;
	instruction.d.pos = pos;
	instruction_dyn_array_push(
		ts->parent_vm,
		&ts->cs->instructions,
		&instruction
	);
}



static void expression(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_next_token(ts);
	read_next_token(ts);
	read_next_token(ts);
}



static void if_statement(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_next_token(ts);
	expression(ts);

}

// Returns true if it's a break, continue or return statement
// These make all following statements unreachable
static sno_Bool statement(Tokenizer* ts) {
	sno_assert_ptr(ts);

	switch (ts->token.type) {
	case TK_IF:
		if_statement(ts);
		return sno_FALSE;

	default:
		syntax_error_at_cur_token(
			ts,
			"Unexpected token"
		);
		break;
	}
}



static void statement_list(Tokenizer* ts) {
	sno_assert_ptr(ts);

	while (1) {
		statement(ts);
	}
}

static void block(Tokenizer* ts) {
	sno_assert_ptr(ts);
	SourceCodePos opening_brace_pos = ts->token.pos;
	if (ts->token.type != TK_LBRACE) {
		syntax_error(
			ts,
			ts->prev_token.pos,
			"Expected the opening brace '{' of a block here"
		);
	}
	read_next_token(ts); // Skip '{'
	//enter_block(ts);
	statement_list(ts);
	if (ts->token.type != TK_RBRACE) {
		syntax_error(
			ts,
			opening_brace_pos,
			"This block is missing its closing brace '}'"
		);
	}
}



static Bytecode* parse_global_scope(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_first_token(ts);

	Compiler cs = { 0 };
	init_function_compiler(ts, &cs);
	
	statement_list(ts);
	if (ts->token.type != TK_EOF) {
		sno_unreachable;
		syntax_error(ts, 0, "Global scope ended early here");
	}
	//emit_instruction_1(ts, sno_I_RETURN, 0);

	Bytecode* bytecode = free_function_compiler(ts, &cs);

#ifdef DEBUG_PRINT_PARSER
	//sno_print_bytecode(cs.bytecode);
#endif

	return bytecode;
}

Bytecode* compile_source_code(
	sno_VMState* vm,
	const IString* source_code_name,
	const IString* source_code
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(source_code_name);
	sno_assert_ptr(source_code);

	sno_Bool success = sno_TRUE;
	Bytecode* bytecode = NULL;
	ExceptionJump exception_jump;
	exception_jump.prev = vm->exception_jump;
	vm->exception_jump = &exception_jump;
	if (setjmp(exception_jump.buf) == 0) {
		Tokenizer ts = { 0 };
		ts.parent_vm = vm;
		ts.source_code_name = source_code_name;
		ts.source_code = source_code;
		ts.source_code_end = istring_chars(source_code) + source_code->length;
		ts.cur_char = istring_chars(source_code);
		ts.token_start = istring_chars(source_code);
		ts.cs = NULL;

		bytecode = parse_global_scope(&ts);
	} else {
		success = sno_FALSE;
	}

	vm->exception_jump = vm->exception_jump->prev;
	return bytecode;
}
