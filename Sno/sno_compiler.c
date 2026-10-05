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
		if (opcode == OP_NOP) { continue; }
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
		cs->instructions.count * 2 * sizeof(Instruction),
		instructions,
		pc * sizeof(Instruction)
	);
	instruction_positions = state_realloc(
		state,
		cs->instructions.count * 2 * sizeof(SourceCodePos),
		instruction_positions,
		pc * sizeof(SourceCodePos)
	);
	bytecode->num_instructions = pc;
	bytecode->instructions = instructions;
	bytecode->instruction_positions = instruction_positions;
}

static Bytecode* free_function_compiler(Tokenizer* ts, Compiler* cs) {
	sno_GlobalState* state = ts->parent_vm->state;

	Bytecode* bytecode = create_bytecode(ts->parent_vm);
	bytecode->name = cs->name;
	bytecode->source_code_name = ts->source_code_name;
	bytecode->source_code = ts->source_code;

	sno_assert(cs->instructions.count < MAX_BYTECODE_INSTRUCTIONS);
	compact_instructions(cs, bytecode);
	
	sno_assert(cs->number_constants.count < MAX_NUMBER_CONSTANTS);
	bytecode->number_constants = state_alloc(
		state,
		cs->number_constants.count * sizeof(sno_Number)
	);
	bytecode->num_number_constants = (ConstID)cs->number_constants.count;
	memcpy(
		bytecode->number_constants,
		cs->number_constants.buffer,
		cs->number_constants.count * sizeof(sno_Number)
	);
	number_dyn_array_clear(ts->parent_vm, &cs->number_constants);

	sno_assert(cs->string_constants.count < MAX_STRING_CONSTANTS);
	bytecode->string_constants = state_alloc(
		state,
		cs->string_constants.count * sizeof(sno_Number)
	);
	bytecode->num_string_constants = (ConstID)cs->string_constants.count;
	memcpy(
		bytecode->string_constants,
		cs->string_constants.buffer,
		cs->string_constants.count * sizeof(sno_Number)
	);
	istring_dyn_array_clear(ts->parent_vm, &cs->string_constants);

	ts->cs = cs->parent_function;
	return bytecode;
}



static void expression(Tokenizer* ts);
static int open_expression_list(Tokenizer* ts);
static int closed_expression_list(
	Tokenizer* ts,
	TokenType closing_token,
	int max_exprs
);
static void block(Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global);
static void brace_block(Tokenizer* ts, sno_Bool is_loop);



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



static PC emit_instruction(Tokenizer* ts, CompilerInstruction instruction) {
	sno_assert_ptr(ts);
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

static PC emit(
	Tokenizer* ts,
	SourceCodePos pos,
	OpCode opcode,
	uint16_t arg
) {
	sno_assert_ptr(ts);
	CompilerInstruction instruction = { 0 };
	instruction.d.opcode = opcode;
	instruction.d.arg = arg;
	instruction.d.pos = pos;
	return emit_instruction(ts, instruction);
}

static PC emit_number(
	Tokenizer* ts,
	SourceCodePos pos,
	sno_Number number
) {
	sno_assert_ptr(ts);
	return emit(ts, pos, OP_NUMBER, add_number_constant(ts->cs, number));
}

static PC emit_string(
	Tokenizer* ts,
	SourceCodePos pos,
	IString* string
) {
	sno_assert_ptr(ts);
	return emit(ts, pos, OP_STRING, add_string_constant(ts->cs, string));
}

static CompilerInstruction* get_last_instruction(const Compiler* cs) {
	sno_assert_ptr(cs);
	sno_assert(cs->instructions.count > 0);
	return &cs->instructions.buffer[
		cs->instructions.count - 1
	];
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
		emit(cs->ts, NO_POS, OP_GET_LOCAL, (LocalSlot)id);
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
			name.pos,
			OP_GET_GLOBAL,
			add_string_constant(ts->cs, name.info.string)
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



static void linalg_constructor(Tokenizer* ts) {
	SourceCodePos pos = ts->token.pos;
	TokenType type = ts->token.type - TK_VEC2;
	read_next_token(ts);
	if (ts->token.type != TK_LPAREN) {
		syntax_error_at_cur_token(
			ts,
			"Expected an opening parenthesis '('"
		);
	}
	uint32_t num_values = closed_expression_list(ts, TK_RPAREN, 16);
	emit(ts, pos, OP_NEW_LINALG, (uint16_t)(type | num_values));
}

static void operand_primary(Tokenizer* ts) {
	switch (ts->token.type) {
	case TK_NONE: {
		emit(ts, ts->token.pos, OP_NONE, 0);
	} break;
	case TK_TRUE: {
		emit(ts, ts->token.pos, OP_BOOL, 1);
	} break;
	case TK_FALSE: {
		emit(ts, ts->token.pos, OP_BOOL, 0);
	} break;
	case TK_NUMBER: {
		emit_number(ts, ts->token.pos, ts->token.info.number);
	} break;
	case TK_STRING: {
		emit_string(ts, ts->token.pos, ts->token.info.string);
	} break;
	case TK_VEC2:
	case TK_VEC3:
	case TK_VEC4:
	case TK_QUAT:
	case TK_MAT2:
	case TK_MAT3:
	case TK_MAT4: {
		linalg_constructor(ts);
	} break;
	case TK_IDENTIFIER: {
		identifier(ts, ts->token);
	} break;
	case TK_SELF: {
		sno_not_implemented;
	} break;
	case TK_LPAREN: {
		SourceCodePos lparen_pos = ts->token.pos;
		read_next_token(ts);
		expression(ts);
		if (ts->token.type != TK_RPAREN) {
			syntax_error(
				ts,
				lparen_pos,
				"Missing closing parenthesis"
			);
		}
		read_next_token(ts);
		return;
	} break;
	default:
		syntax_error_at_cur_token(
			ts,
			"Expected an operand"
		);
		break;
	}
	read_next_token(ts);
}

static int arguments(Tokenizer* ts) {
	sno_assert_ptr(ts);
	sno_assert(ts->token.type == TK_LPAREN);
	read_next_token(ts);
	return closed_expression_list(ts, TK_RPAREN, MAX_EXPR_PER_STMT);
}

static void call(Tokenizer* ts, SourceCodePos pos) {
	sno_assert_ptr(ts);
	int num_args = arguments(ts);
	CompilerInstruction i = { 0 };
	i.call.pos = pos;
	i.call.opcode = OP_CALL;
	i.call.argc = (uint8_t)num_args;
	i.call.retc = 1;
	emit_instruction(ts, i);
}

static void operand_postfix(Tokenizer* ts) {
	sno_assert_ptr(ts);
	SourceCodePos pos = ts->token.pos;
	while (1) {
		switch (ts->token.type) {
		case TK_LBRACKET: { // Index
			read_next_token(ts);
			expression(ts);
			if (ts->token.type != TK_RBRACKET) {
				syntax_error(ts, pos, "Missing closing bracket ']'");
			}
			read_next_token(ts);
			emit(ts, pos, OP_GET_INDEX, 0);
		} break;
		case TK_DOT: { // Field or method call
			read_next_token(ts);
			if (ts->token.type != TK_IDENTIFIER) {
				syntax_error(ts, pos, "Missing closing bracket ']'");
			}
			ConstID name_const_id = add_string_constant(ts->cs, ts->token.info.string);
			read_next_token(ts);
			if (ts->token.type == TK_LPAREN) {
				emit(ts, pos, OP_GET_METHOD, name_const_id);
				call(ts, ts->token.pos);
			} else {
				emit(ts, ts->token.pos, OP_GET_FIELD, name_const_id);
			}
		} break;
		case TK_LPAREN: { // Function call
			emit(ts, pos, OP_NONE, 0); // Self parameter = null
			call(ts, ts->token.pos);
		} break;
		default: {
			return;
		}
		}
	}
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
		emit(ts, unop_pos, OP_UNOP, (uint16_t)unary_op);
	} else {
		operand(ts);
	}
	BinOp binop = get_binop(ts->token.type);
	while (binop != NOT_BINOP && operator_precedence[binop].left > precedence) {
		BinOp next_binop;
		SourceCodePos binop_pos = ts->token.pos;
		read_next_token(ts);
		if (binop == BINOP_LAND || binop == BINOP_LOR) {
			PC jump_from = emit(
				ts,
				binop_pos,
				binop == BINOP_LAND ? OP_AND : OP_OR,
				0
			);
			next_binop = subexpression(ts, operator_precedence[binop].right);
			PC jump_to = emit(ts, NO_POS, OP_TO_BOOL, 0) + 1;
			if (jump_to - jump_from > UINT16_MAX) {
				syntax_error(
					ts,
					binop_pos,
					"Jumps too far"
				);
			}
			ts->cs->instructions.buffer[jump_from].d.arg =
				(uint16_t)(jump_to - jump_from);
			next_binop = NOT_BINOP;
		} else {
			next_binop = subexpression(ts, operator_precedence[binop].right);
			emit(ts, binop_pos, OP_BINOP, (uint16_t)binop);
		}
		binop = next_binop;
	}
	return binop;
}

static void expression(Tokenizer* ts) {
	sno_assert_ptr(ts);
	subexpression(ts, 0);
}

static int open_expression_list(Tokenizer* ts) {
	sno_assert_ptr(ts);
	int num_expressions = 1;
	while (1) {
		expression(ts);
		if (ts->token.type == TK_TERMINATOR) {
			break;
		} else if (ts->token.type == TK_COMMA) {
			num_expressions++;
			read_next_token(ts);
			continue;
		} else {
			syntax_error_at_cur_token(
				ts,
				"Expected a comma ',' or statement end"
			);
		}
	}
	return num_expressions;
}

static int closed_expression_list(
	Tokenizer* ts,
	TokenType closing_token, 
	int max_exprs
) {
	sno_assert_ptr(ts);
	if (ts->token.type == closing_token) {
		read_next_token(ts);
		return 0;
	}
	int num_exprs = 1;
	while (1) {
		expression(ts);
		if (ts->token.type == TK_COMMA) {
			if (num_exprs > max_exprs) {
				syntax_error_at_cur_token(
					ts,
					"Too many expressions"
				);
			}
			read_next_token(ts);
			if (ts->token.type == closing_token) {
				break;
			}
			num_exprs++;
			continue;
		} else if (ts->token.type == closing_token) {
			break;
		} else {
			syntax_error_at_cur_token(
				ts,
				"Expected a comma ',' or a closing parenthesis ')'"
			);
		}
	}
	sno_assert(ts->token.type == closing_token);
	read_next_token(ts); // Skip closing token
	return num_exprs;
}



static void if_statement(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_next_token(ts);
	expression(ts); // Condition
	PC jump_from = emit(ts, NO_POS, OP_JUMP_FRWD_IF_FALSE, 0);
	SourceCodePos lbrace_pos = ts->token.pos;
	brace_block(ts, sno_FALSE);
	PC jump_to = (PC)(ts->cs->instructions.count);
	if (ts->token.type == TK_ELSE) {
		SourceCodePos else_pos = ts->token.pos;
		jump_to++;
		read_next_token(ts);
		PC else_jump_from = emit(ts, NO_POS, OP_JUMP_FRWD, 0);
		if (ts->token.type == TK_IF) {
			if_statement(ts);
		} else {
			brace_block(ts, sno_FALSE);
		}
		PC else_jump_to = (PC)ts->cs->instructions.count;
		if (else_jump_to - else_jump_from > UINT16_MAX) {
			syntax_error(ts, else_pos, "Jumps too far");
		}
		ts->cs->instructions.buffer[else_jump_from].d.arg =
			(uint16_t)(else_jump_to - else_jump_from);
	}
	if (jump_to - jump_from > UINT16_MAX) {
		syntax_error(ts, lbrace_pos, "Jumps too far");
	}
	ts->cs->instructions.buffer[jump_from].d.arg =
		(uint16_t)(jump_to - jump_from);
}

static void while_statement(Tokenizer* ts) {
	sno_assert_ptr(ts);
	read_next_token(ts);
	PC back_to = (PC)(ts->cs->instructions.count);
	expression(ts); // Condition
	PC condition_jump_from = emit(ts, NO_POS, OP_JUMP_FRWD_IF_FALSE, 0);
	brace_block(ts, sno_TRUE);
	PC back_from = emit(ts, NO_POS, OP_JUMP_BACK, 0);
	PC condition_jump_to = back_from + 1; // TODO: Maybe check this increment?
	ts->cs->instructions.buffer[back_from].d.arg =
		(uint16_t)(back_from - back_to);
	ts->cs->instructions.buffer[condition_jump_from].d.arg =
		(uint16_t)(condition_jump_to - condition_jump_from);
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
			emit(ts, NO_POS, OP_NONE, 0);
		}
	} else {
		sno_assert(
			num_declarations >= 1 &&
			num_declarations <= MAX_EXPR_PER_STMT
		);
		sno_assert(ts->prev_token.type == TK_ASSIGN);
		int num_expressions = open_expression_list(ts);
		if (num_expressions > (int)num_declarations) {
			syntax_error(
				ts,
				assignment_token_pos,
				"There %s %i item%s on the left but %i items on the right",
				num_declarations == 1 ? "is" : "are",
				(int)num_declarations,
				num_declarations == 1 ? "" : "s",
				num_expressions
			);
		}
	}
	sno_assert(num_declarations >= 1 && num_declarations <= MAX_EXPR_PER_STMT);
	for (int i = (int)num_declarations - 1; i >= 0; i--) {
		Token name_token = name_tokens[i];
		if (ts->cs->current_block->is_global) {
			emit(
				ts,
				name_token.pos,
				OP_SET_NEW_GLOBAL,
				add_string_constant(ts->cs, name_token.info.string)
			);
		} else {
			LocalSlot local_slot = try_declare_local_variable(ts, name_token);
			emit(ts, NO_POS, OP_SET_LOCAL, local_slot);
		}
	}

}

static sno_Bool is_lvalue(OpCode opcode) {
	return
		opcode == OP_GET_LOCAL ||
		opcode == OP_GET_GLOBAL ||
		opcode == OP_GET_FIELD ||
		opcode == OP_GET_INDEX;
}

static void expression_statement(Tokenizer* ts) {
	sno_assert_ptr(ts);
	BinOp assignment_op = NOT_BINOP;
	SourceCodePos assignment_pos = 0;
	sno_Bool may_need_mash = sno_FALSE;
	int num_lhs = 1;
	PC lhs_instructions[MAX_EXPR_PER_STMT];
	while (1) {
		expression(ts);
		CompilerInstruction last_i = *get_last_instruction(ts->cs);
		// Save the lhs instructions on the stack,
		// to convert them to SET later
		if (last_i.d.opcode == OP_GET_FIELD) {
			may_need_mash = sno_TRUE;
		} else if (last_i.d.opcode == OP_GET_INDEX) {
			may_need_mash = sno_TRUE;
		}
		lhs_instructions[num_lhs - 1] = (PC)ts->cs->instructions.count - 1;
		if (token_is_assignment(ts->token.type)) {
			if (ts->token.type != TK_ASSIGN) {
				// Assign op
				if (num_lhs > 1) {
					syntax_error_at_cur_token(
						ts,
						"Assign ops are only valid on singular operands"
					);
				}
				// Need to copy the operands for both GET and SET
				if (last_i.d.opcode == OP_GET_FIELD) {
					ts->cs->instructions.count--;
					emit(ts, NO_POS, OP_COPY, 1);
					emit_instruction(ts, last_i);
					lhs_instructions[0]++;
				} else if (last_i.d.opcode == OP_GET_INDEX) {
					ts->cs->instructions.count--;
					emit(ts, NO_POS, OP_COPY, 2);
					emit_instruction(ts, last_i);
					lhs_instructions[0]++;
				}
			}
			assignment_op = ts->token.type - 1 - TK_ASSIGN;
			assignment_pos = ts->token.pos;
			read_next_token(ts);
			break;
		} else if (ts->token.type == TK_COMMA) {
			num_lhs++;
			if (num_lhs > MAX_EXPR_PER_STMT) {
				syntax_error_at_cur_token(
					ts,
					"Too many expressions in one statement"
				);
			}
			read_next_token(ts);
			continue;
		} else if (ts->token.type == TK_TERMINATOR) {
			// No assignment
			if (num_lhs > 1) {
				syntax_error(
					ts,
					ts->prev_token.pos,
					"Multiple expressions aren't allowed here"
				);
			}
			if (last_i.d.opcode != OP_CALL) {
				syntax_error(
					ts,
					ts->prev_token.pos,
					"Statement performs redundant operations"
				);
			}
			get_last_instruction(ts->cs)->call.retc = 0;
			// Check if in REPL mode
			return;
		} else {
			syntax_error_at_cur_token(
				ts,
				"Expected either comma ',' or an assignment token"
			);
		}
	}
	int num_rhs = open_expression_list(ts);
	CompilerInstruction* last_instruction = get_last_instruction(ts->cs);
	if (num_rhs > num_lhs) {
		syntax_error(
			ts,
			assignment_pos,
			"There are more expressions on the right than on the left"
		);
	}
	if (last_instruction->d.opcode == OP_CALL) {
		// Correct the number of returns
		sno_assert(num_lhs >= num_rhs);
		last_instruction->call.retc = (uint8_t)(num_lhs - num_rhs + 1);
	} else {
		if (num_lhs != num_rhs) {
			sno_assert(num_lhs > num_rhs);
			syntax_error(
				ts,
				assignment_pos,
				"There are more expressions on the left than on the right"
			);
		}
	}



	// Now it's time to assign
	if (assignment_op != NOT_BINOP) {
		sno_assert(num_lhs == 1);
		CompilerInstruction get = ts->cs->instructions.buffer[
			lhs_instructions[0]
		];
		if (!is_lvalue(get.d.opcode)) {
			syntax_error(
				ts,
				get.d.pos,
				"This operand is not assignable"
			);
		}
		get.d.opcode++; // Convert GET instruction to SET instruction
		sno_assert(assignment_op >= BINOP_ADD && assignment_op < NUM_BINOPS);
		emit(ts, assignment_pos, OP_BINOP, (uint16_t)assignment_op);
		emit_instruction(ts, get);
	} else {
		if (num_lhs > 1 && may_need_mash) {
			emit(ts, NO_POS, OP_MASH, (uint16_t)num_lhs);
		}
		for (int i = num_lhs - 1; i >= 0; i--) {
			CompilerInstruction* get = &ts->cs->instructions.buffer[
				lhs_instructions[i]
			];
			if (!is_lvalue(get->d.opcode)) {
				syntax_error(
					ts,
					get->d.pos,
					"This operand is not assignable"
				);
			}
			CompilerInstruction set = *get;
			set.d.opcode++;
			get->d.opcode = OP_NOP; // Remove the GET instruction
			emit_instruction(ts, set); // Put in the SET instruction
		}
	}
}

// return_stmt ::= 'return' expr_list_open
static void return_statement(Tokenizer* ts) {
	SourceCodePos pos = ts->token.pos;
	read_next_token(ts);
	int num_returns;
	if (ts->token.type == TK_TERMINATOR) {
		// No returns
		num_returns = 0;
	} else {
		num_returns = open_expression_list(ts);
	}
	sno_assert(num_returns >= 0 && num_returns <= MAX_EXPR_PER_STMT);
	emit(ts, pos, OP_RETURN, (uint16_t)num_returns);
}

// Returns true if it's a break, continue or return statement
// These make all following statements unreachable
static sno_Bool statement(Tokenizer* ts) {
	sno_assert_ptr(ts);

	switch (ts->token.type) {
	case TK_IF:
		if_statement(ts);
		return sno_FALSE;
	case TK_WHILE:
		while_statement(ts);
		return sno_FALSE;
	case TK_RETURN:
		return_statement(ts);
		return sno_TRUE;
	case TK_VAR:
	case TK_CONST:
		declaration_statement(ts);
		return sno_FALSE;
	default:
		expression_statement(ts);
		return sno_FALSE;
	/*default:
		syntax_error_at_cur_token(
			ts,
			"Unexpected token at the start of a statement"
		);
		break;*/
	}
}



static void block(Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global) {
	sno_assert_ptr(ts);
	sno_assert(!(is_loop && is_global));
	Block block;
	enter_block(ts->cs, &block, is_loop, is_global);
	while (1) {
		if (ts->token.type == TK_EOF || ts->token.type == TK_RBRACE) {
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
	read_next_token(ts);
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
	emit(ts, NO_POS, OP_RETURN, 0);
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
