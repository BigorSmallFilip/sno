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

static void init_function_compiler(Tokenizer* ts, Compiler* cs, IString* name) {
	sno_assert(ts);
	//sno_VMState* vm = ts->parent_vm;
	instruction_dyn_array_init(&cs->instructions);
	number_dyn_array_init(&cs->number_constants);
	istring_dyn_array_init(&cs->string_constants);
	bytecode_dyn_array_init(&cs->bytecode_constants);

	ts->cs = cs;
	cs->ts = ts;
	cs->name = name;
}

static void compact_instructions(Compiler* cs, Bytecode* bytecode) {
	sno_assert_ptr(cs);
	sno_assert_ptr(bytecode);
	sno_assert(cs->instructions.count > 0);
	sno_GlobalState* state = cs->ts->parent_vm->state;
	PC pc = 0;
	Instruction* instructions = state_alloc(
		state,
		cs->instructions.count * 2 * sizeof(Instruction)
	);
	SourceCodePos* instruction_positions = state_alloc(
		state,
		cs->instructions.count * sizeof(SourceCodePos)
	);
	for (size_t i = 0; i < cs->instructions.count; i++) {
		OpCode opcode = cs->instructions.buffer[i].d.opcode;
		uint16_t arg = cs->instructions.buffer[i].d.arg;
		SourceCodePos pos = cs->instructions.buffer[i].d.pos;
		if (arg < 0xFF) {
			instruction_positions[pc] = pos;
			instructions[pc++] = opcode | (arg << 8);
		} else {
			instruction_positions[pc] = pos;
			instructions[pc++] = opcode | 0xFF;
			instruction_positions[pc] = pos;
			instructions[pc++] = arg;
		}
	}
	instructions = state_realloc(
		state,
		cs->instructions.count * 2 * sizeof(SourceCodePos),
		instructions,
		pc * sizeof(SourceCodePos)
	);
	state_realloc(
		state,
		cs->instructions.count * 2 * sizeof(Instruction),
		instruction_positions,
		pc * sizeof(Instruction)
	);
	bytecode->num_instructions = pc;
	bytecode->instructions = instructions;
	bytecode->instruction_positions = instruction_positions;
}

static Bytecode* free_function_compiler(Tokenizer* ts, Compiler* cs) {
	Bytecode* bytecode = create_bytecode(ts->parent_vm);
	bytecode->name = cs->name;
	bytecode->source_code_name = ts->source_code_name;
	bytecode->source_code = ts->source_code;
	compact_instructions(cs, bytecode);


	ts->cs = cs->parent_function;
	return bytecode;
}



static void expression(Tokenizer* ts);
static void block(Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global);



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
	sno_assert(to_id >= 0 && to_id <= MAX_ACTIVE_LOCAL_VARS);
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



static void enter_block(
	Compiler* cs,
	Block* block,
	sno_Bool is_loop,
	sno_Bool is_global
) {
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




static void operand_primary(Tokenizer* ts) {
	switch (ts->token.type) {
	case TK_NUMBER: {
		emit(
			ts,
			OP_NUMBER,
			add_number_constant(ts->cs, ts->token.info.number),
			ts->token.pos
		);
	} break;
	default:
		break;
	}
	read_next_token(ts);
}

static void operand_postfix(Tokenizer* ts) {
	sno_assert_ptr(ts);
}

static void operand(Tokenizer* ts) {
	operand_primary(ts);
	operand_postfix(ts);
}



static UnOp get_unop(TokenType tokentype) {
	switch (tokentype) {
	case TK_SUB: return UNOP_NEG;
	case TK_BITFLIP: return UNOP_BITFLIP;
	case TK_LNOT: return UNOP_LNOT;
	default: return NOT_UNOP;
	}
}

static BinOp get_binop(TokenType tokentype) {
	switch (tokentype) {
	case TK_ADD: return BINOP_ADD;
	case TK_SUB: return BINOP_SUB;
	case TK_MUL: return BINOP_MUL;
	case TK_DIV: return BINOP_DIV;
	case TK_IDIV: return BINOP_IDIV;
	case TK_MOD: return BINOP_MOD;
	case TK_POW: return BINOP_POW;
	case TK_BAND: return BINOP_BAND;
	case TK_BOR: return BINOP_BOR;
	case TK_BXOR: return BINOP_BXOR;
	case TK_SHL: return BINOP_SHL;
	case TK_SHR: return BINOP_SHR;
	case TK_EQ: return BINOP_EQ;
	case TK_NEQ: return BINOP_NEQ;
	case TK_LT: return BINOP_LT;
	case TK_GT: return BINOP_GT;
	case TK_LE: return BINOP_LE;
	case TK_GE: return BINOP_GE;
	case TK_LAND: return BINOP_LAND;
	case TK_LOR: return BINOP_LOR;
	default: return NOT_BINOP;
	}
}

static const struct {
	uint8_t left;  // Left precedence for each binary operator
	uint8_t right; // Right precedence
} operator_precedence[] = {
	{6, 6}, {6, 6}, {7, 7}, {7, 7}, {7, 7}, {7, 7},  // '+' '-' '*' '/' '//' '%'
	{10, 9}, // '**' (right associative)
	{3, 3}, {3, 3}, {3, 3}, // '&' '|' '^'
	{5, 5}, {5, 5}, // '<<' '>>'
	{4, 4}, {4, 4}, // '==' '!='
	{4, 4}, {4, 4}, {4, 4}, {4, 4}, // '<' '>' '<=' '>='
	{2, 2}, {1, 1}, // '&&' '||'
};
#define UNOP_PRECEDENCE 8 // priority for unary operators

static BinOp subexpression(
	Tokenizer* ts,
	unsigned int precedence
) {
	UnOp unary_op = get_unop(ts->token.type);
	if (unary_op != NOT_UNOP) {
		// Unary op
		SourceCodePos unop_pos = ts->token.pos;
		read_next_token(ts);
		subexpression(ts, UNOP_PRECEDENCE);
		emit(ts, OP_UNOP, (uint16_t)unary_op, unop_pos);
	} else {
		operand(ts);
	}
	BinOp binop = get_binop(ts->token.type);
	while (binop != NOT_BINOP && operator_precedence[binop].left > precedence) {
		BinOp next_binop;
		SourceCodePos binop_pos = ts->token.pos;
		read_next_token(ts);
		if (binop == BINOP_LAND || binop == BINOP_LOR) {
			/*uint32_t jump_from = emit_instruction(
				ts,
				binop == BINOP_LAND ? OP_AND : OP_OR
			);
			next_binop = subexpression(ts, operator_precedence[binop].right);
			uint32_t jump_to = emit_instruction(ts, OP_TO_BOOL) + 1;
			set_jump_dst(ts, jump_from, jump_to);*/
			next_binop = NOT_BINOP;
		} else {
			next_binop = subexpression(ts, operator_precedence[binop].right);
			emit(ts, OP_BINOP, (uint16_t)binop, binop_pos);
		}
		binop = next_binop;
	}
	return binop;
}

static void expression(Tokenizer* ts) {
	sno_assert_ptr(ts);
	subexpression(ts, 0);
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
//                      ( ( '=' expr_list_open ) | ';' )
static void declaration_statement(Tokenizer* ts) {
	sno_assert_ptr(ts);
	sno_Bool is_const = ts->token.type == TK_CONST;
	read_next_token(ts);
	sno_Bool no_assignment = sno_FALSE;
	Token name_tokens[MAX_EXPR_PER_STMT];
	sno_Bool is_consts[MAX_EXPR_PER_STMT];
	size_t num_declarations = 0;
	SourceCodePos assignment_token_pos = ts->token.pos;
	while (1) {
		if (ts->token.type != TK_IDENTIFIER) {
			syntax_error_at_cur_token(
				ts,
				"Expected a variable name"
			);
		}
		is_consts[num_declarations] = is_const;
		name_tokens[num_declarations] = ts->token;
		num_declarations++;
		if (num_declarations > MAX_EXPR_PER_STMT) {
			syntax_error_at_cur_token(
				ts,
				"Expected a variable name " sno_stringify(MAX_EXPR_PER_STMT)
			);
		}
		read_next_token(ts);
		if (ts->token.type == TK_TERMINATOR) {
			// Don't skip this token
			no_assignment = sno_TRUE;
			break;
		}
		if (ts->token.type == TK_ASSIGN) {
			assignment_token_pos = ts->token.pos;
			read_next_token(ts);
			no_assignment = sno_FALSE;
			break;
		}
		if (ts->token.type != TK_COMMA) {
			syntax_error_at_cur_token(
				ts,
				"The only valid tokens here are ',' '=' or a line end"
			);
		}
		// It's a comma
		read_next_token(ts);
		if (ts->token.type == TK_VAR) {
			is_const = sno_FALSE;
			read_next_token(ts);
		} else if (ts->token.type == TK_CONST) {
			is_const = sno_TRUE;
			read_next_token(ts);
		}
	}

	if (no_assignment) {
		for (size_t i = 0; i < num_declarations; i++) {
			emit(ts, OP_NONE, 0, NO_POS);
		}
	} else {
		sno_assert(num_declarations >= 1);
		sno_assert(ts->prev_token.type == TK_ASSIGN);
		size_t i = 0;
		while (1) {
			expression(ts);
			if (ts->token.type == TK_TERMINATOR) {
				// Fix calls
				break;
			}
			i++;
			if (i > num_declarations) {
				syntax_error(
					ts,
					assignment_token_pos,
					"There %s %i item%s on the left but %i items on the right",
					num_declarations == 1 ? "is" : "are",
					(int)num_declarations,
					num_declarations == 1 ? "" : "s",
					(int)i
				);
			}
		}
	}
	sno_assert(num_declarations >= 1 && num_declarations <= MAX_EXPR_PER_STMT);
	for (int i = (int)num_declarations - 1; i >= 0; i--) {
		Token name_token = name_tokens[i];
		if (ts->cs->current_block->is_global) {
			emit(
				ts,
				OP_SET_NEW_GLOBAL,
				add_string_constant(ts->cs, name_token.info.string),
				name_token.pos
			);
		} else {
			LocalSlot local_slot = try_declare_local_variable(ts, name_token);
			emit(ts, OP_SET_LOCAL, local_slot, NO_POS);
		}
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



static void block(Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global) {
	sno_assert_ptr(ts);
	sno_assert(!(is_loop && is_global));
	Block block;
	enter_block(ts->cs, &block, is_loop, is_global);
	while (1) {
		if (ts->token.type == TK_EOF) {
			break;
		}
		statement(ts);
		if (ts->token.type != TK_TERMINATOR) {
			syntax_error_at_cur_token(ts, "Statement didn't end properly lol");
		}
		read_next_token(ts);
	}
	exit_block(ts->cs);
}

static void brace_block(Tokenizer* ts, sno_Bool is_loop) {
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
	block(ts, is_loop, sno_FALSE);
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
	init_function_compiler(ts, &cs, ts->source_code_name);
	
	block(ts, sno_FALSE, sno_TRUE);
	if (ts->token.type != TK_EOF) {
		sno_unreachable;
		syntax_error(ts, 0, "Global scope ended early here");
	}
	emit(ts, OP_RETURN, 0, NO_POS);
	Bytecode* bytecode = free_function_compiler(ts, &cs);

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

#ifdef DEBUG_PRINT_PARSER
	print_bytecode(bytecode);
#endif

	return bytecode;
}
