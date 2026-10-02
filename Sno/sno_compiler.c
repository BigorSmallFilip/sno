#include "sno_compiler.h"

#define DEBUG_PRINT_PARSER

DEFINE_GENERIC_DYN_ARRAY(CompilerInstruction, Instruction, instruction);
DEFINE_GENERIC_DYN_ARRAY(Bytecode, Bytecode, bytecode);
DEFINE_GENERIC_DYN_ARRAY(LocalVar, LocalVar, local_var);



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



static void expression(Tokenizer* ts);
static void block(Tokenizer* ts);



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



static PC emit(
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
	if (ts->cs->instructions.count > MAX_BYTECODE_INSTRUCTIONS) {
		syntax_error_at_cur_token(
			ts,
			"Compiler generated too many bytecode instructions"
		);
	}
	return (PC)(ts->cs->instructions.count - 1);
}





static LocalSlot register_local_variable(Tokenizer* ts, IString* name) {
	LocalVar local;
	local.name = name;
	local.slot = ts->cs->num_active_local_slots;
	local.start_pc = (PC)ts->cs->instructions.count;
	local.end_pc = 0;
	local_var_dyn_array_push(ts->parent_vm, &ts->cs->local_vars, &local);
	sno_assert(ts->cs->local_vars.count < MAX_ACTIVE_LOCAL_VARS);
	return (LocalSlot)(ts->cs->local_vars.count - 1);
}

static LocalSlot try_declare_local_variable(Tokenizer* ts, Token token) {
	IString* name = token.info.string;
	for (LocalSlot i = 1; i < ts->cs->num_active_local_slots; i++) {
		LocalID local_id = ts->cs->active_local_vars[i];
		const LocalVar* local = &ts->cs->local_vars.buffer[local_id];
		if (local->name == name) {
			syntax_error(
				ts,
				token.pos,
				"A local variable called '%.*s' already exists",
				(unsigned int)name->length,
				istring_chars(name)
			);
		}
	}
	if (ts->cs->num_active_local_slots > MAX_ACTIVE_LOCAL_VARS) {
		syntax_error(
			ts,
			token.pos,
			"This function has too many local variables."
		);
	}
	LocalID local_id = register_local_variable(ts, name);
	ts->cs->active_local_vars[ts->cs->num_active_local_slots] = local_id;
	ts->cs->num_active_local_slots++;
	if (ts->cs->num_active_local_slots > ts->cs->max_active_local_slots) {
		ts->cs->max_active_local_slots = ts->cs->num_active_local_slots;
	}
	return ts->cs->num_active_local_slots - 1;
}

static void deactivate_local_variables(Compiler* cs, LocalSlot to_id) {
	sno_assert(to_id >= 1 && to_id <= MAX_ACTIVE_LOCAL_VARS);
	for (int i = cs->num_active_local_slots - 1; i >= (int)to_id; i--) {
		cs->local_vars.buffer[i].end_pc = (PC)cs->instructions.count;
	}
	cs->num_active_local_slots = to_id;
}

static int search_local_variable_in_function(Compiler* cs, IString* name) {
	sno_assert(cs->num_active_local_slots < MAX_ACTIVE_LOCAL_VARS);
	for (int i = cs->num_active_local_slots - 1; i >= 1; i--) {
		LocalVar* local = &cs->local_vars.buffer[cs->active_local_vars[i]];
		if (local->name == name) {
			return i;
		}
	}
	return -1;
}

static sno_Bool recursive_search_local_variable(Compiler* cs, IString* name) {
	if (cs->current_block->is_global) {
		// If you've reached the global scope then stop searching
		return sno_FALSE;
	}
	int id = search_local_variable_in_function(cs, name);
	if (id >= 0) {
		// Id 0 should always be the 'self' argument
		sno_assert(id >= 1 && id < MAX_ACTIVE_LOCAL_VARS);
		emit(cs->ts, OP_GET_LOCAL, (LocalSlot)id, NO_POS);
		return sno_TRUE;
	} else {
		if (
			!cs->parent_function ||
			!recursive_search_local_variable(cs->parent_function, name)
		) {
			return sno_FALSE;
		} else {
			// Upval found
			sno_not_implemented;
		}
	}
	sno_unreachable;
	return sno_FALSE;
}

static void identifier(Tokenizer* ts, Token name) {
	if (!recursive_search_local_variable(ts->cs, name.info.string)) {
		// Nothing found so treat it like a global
		emit(
			ts,
			OP_GET_GLOBAL,
			add_string_constant(ts->cs, name.info.string),
			name.pos
		);
	}
}



static void enter_block(Compiler* cs, Block* block, sno_Bool is_loop, sno_Bool is_global) {
	sno_assert(!(is_loop && is_global));
	block->is_loop = is_loop;
	block->is_global = is_global;
	block->num_active_local_vars = cs->num_active_local_slots;
	block->prev = cs->current_block;
	cs->current_block = block;
	cs->current_block_depth++;
	if (cs->current_block_depth > MAX_BLOCK_DEPTH) {
		syntax_error_at_cur_token(
			cs->ts,
			"Syntax is too deeply nested"
		);
	}
}

static void exit_block(Compiler* cs) {
	Block* block = cs->current_block; // The block to exit out of
	cs->current_block = block->prev;
	cs->current_block_depth--;
	deactivate_local_variables(cs, block->num_active_local_vars);
}





static void expression(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_next_token(ts);
	read_next_token(ts);
	read_next_token(ts);
}

static void open_expression_list(Tokenizer* ts) {
	sno_assert_ptr(ts);
	expression(ts);
}



static void if_statement(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_next_token(ts);
	expression(ts);

}

// declaration_stmt ::= declarator identifier
//                      { ',' [declarator] identifier }
//                      assign expr_list_open
static void declaration_statement(Tokenizer* ts) {
	//sno_Bool is_const = ts->token.type == TK_CONST;
	read_next_token(ts);
	if (ts->token.type != TK_IDENTIFIER) {
		syntax_error_at_cur_token(
			ts,
			"Expected a variable name"
		);
	}
	//IString* name = ts->token.info.string;
	while (1) {

	}
}

// return_stmt ::= 'return' expr_list_open
static void return_statement(Tokenizer* ts) {
	read_next_token(ts);
	
}

// Returns true if it's a break, continue or return statement
// These make all following statements unreachable
static sno_Bool statement(Tokenizer* ts) {
	sno_assert_ptr(ts);

	switch (ts->token.type) {
	case TK_IF:
		if_statement(ts);
		return sno_FALSE;
	case TK_VAR:
	case TK_CONST:
		declaration_statement(ts);
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
	emit(ts, OP_RETURN, 0, NO_POS);
	Bytecode* bytecode = free_function_compiler(ts, &cs);

#ifdef DEBUG_PRINT_PARSER
	//sno_print_bytecode(cs.bytecode);
#endif

	return bytecode;
}

Bytecode* compile_source_code(
	sno_VMState* vm,
	IString* source_code_name,
	IString* source_code
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
