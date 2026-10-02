#include "sno_compiler.h"

#include "sno_vm.h"

static Bytecode* parse_source_code(Tokenizer* ts) {
	sno_assert_ptr(ts);
	return NULL;
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

		bytecode = parse_source_code(&ts);
	} else {
		success = sno_FALSE;
	}

	vm->exception_jump = vm->exception_jump->prev;
	return bytecode;
}
