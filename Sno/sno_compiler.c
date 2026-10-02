#include "sno_compiler.h"

#define DEBUG_PRINT_PARSER

DEFINE_GENERIC_DYN_ARRAY(CompilerInstruction, Instruction, instruction);

static Bytecode* create_bytecode(sno_VMState* vm) {
	Bytecode* bytecode = state_alloc(vm->state, sizeof(Bytecode));
	memset(bytecode, 0, sizeof(Bytecode));
	return bytecode;
}

static void init_function_compiler(Tokenizer* ts, Compiler* cs) {
	sno_assert(ts);
	//sno_VMState* vm = ts->parent_vm;
	

	cs->ts = ts;
	
}

static Bytecode* free_function_compiler(Tokenizer* ts, Compiler* cs) {
	
	ts->cs = cs->parent_function;
	return create_bytecode(ts->parent_vm);
}



static void statement_list(Tokenizer* ts) {
	sno_assert_ptr(ts);
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
	const IString* source_code,
	const IString* source_code_name
) {
	sno_assert_ptr(vm);
	sno_assert_ptr(source_code);
	sno_assert_ptr(source_code_name);

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
