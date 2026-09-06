#include "sno_parser.h"

#include "sno_state.h"
#include "sno_vm.h"
#include <string.h>



static uint8_t add_number_constant(sno_Compiler* cs, sno_Number number) {
	sno_State* state = cs->ts->main_state;
	if (cs->number_constants.count > sno_MAX_NUMBER_CONSTANTS) {
		sno_throw_syntax_error_at_token(
			cs->ts,
			&cs->ts->cur_token,
			"There are too many numbers in this function"
		);
	}
	for (size_t i = 0; i < cs->number_constants.count; i++) {
		sno_Number c = *(sno_Number*)sno_dynarray_get(state, &cs->number_constants, sizeof(sno_Number), i);
		if (c == number) {
			return i;
		}
	}
	sno_ConstID const_id = (sno_ConstID)cs->number_constants.count;
	sno_dynarray_push_back(state, &cs->number_constants, sizeof(sno_Number), &number);
	return const_id;
}

static uint8_t add_string_constant(sno_Compiler* cs, const sno_String* string) {
	sno_State* state = cs->ts->main_state;
	if (cs->string_constants.count > sno_MAX_NUMBER_CONSTANTS) {
		sno_throw_syntax_error_at_token(
			cs->ts,
			&cs->ts->cur_token,
			"There are too many strings in this function"
		);
	}
	for (size_t i = 0; i < cs->string_constants.count; i++) {
		const sno_String* c = (sno_String*)sno_dynarray_get_ptr(state, &cs->string_constants, i);
		if (c == string) {
			return i;
		}
	}
	sno_ConstID const_id = (sno_ConstID)cs->string_constants.count;
	sno_dynarray_push_back_ptr(state, &cs->string_constants, string);
	return const_id;
}

static uint8_t add_sub_function(sno_Compiler* cs, sno_Bytecode* bytecode) {
	sno_Assert(cs->sub_functions.count < 256);
	uint8_t sub_function_index = cs->sub_functions.count;
	sno_DynArray_PushBackPtr(&cs->sub_functions, bytecode);
	return sub_function_index;
}



static uint32_t emit_instruction(sno_Tokenizer* ts, sno_Instruction i) {
	sno_State* state = ts->main_state;
	sno_Compiler* cs = ts->cs;
	sno_dynarray_push_back(state, &cs->instructions, sizeof(sno_Instruction), &i);
	sno_dynarray_push_back(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), &ts->cur_token.source_code);
	return cs->instructions.count - 1;
}

static uint32_t emit_instruction_1(sno_Tokenizer* ts, uint8_t op, uint8_t arg) {
	sno_State* state = ts->main_state;
	sno_Compiler* cs = ts->cs;
	sno_Instruction i = op | (arg << 8);
	sno_dynarray_push_back(state, &cs->instructions, sizeof(sno_Instruction), &i);
	sno_dynarray_push_back(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), &ts->cur_token.source_code);
	return cs->instructions.count - 1;
}

static uint32_t emit_instruction_number(sno_Tokenizer* ts, sno_Number number) {
	return emit_instruction_1(ts, sno_I_LOAD_NUMBER, add_number_constant(ts->cs, number));
}

static uint32_t emit_instruction_string(sno_Tokenizer* ts, const sno_String* string) {
	return emit_instruction_1(ts, sno_I_LOAD_STRING, add_string_constant(ts->cs, string));
}



static void enter_block(sno_Compiler* cs, sno_Block* block, sno_Bool is_loop, sno_Bool is_global) {
	sno_assert(!(is_loop && is_global));
	block->is_loop = is_loop;
	block->is_global = is_global;
	block->num_active_local_vars = cs->num_active_local_vars;
	block->prev = cs->current_block;
	cs->current_block = block;
	cs->current_block_depth++;
	if (cs->current_block_depth > sno_MAX_BLOCK_DEPTH) {
		sno_throw_syntax_error(
			cs->ts,
			cs->ts->cur_char,
			1,
			cs->ts->line,
			cs->ts->column,
			"Function compiles too deeply. Consider breaking it up into multiple separate functions"
		);
	}
}

static void exit_block(sno_Compiler* cs) {
	sno_Block* block = cs->current_block; // The block to exit out of
	cs->current_block = block->prev;
	cs->current_block_depth--;
	//deactivate_local_variables(cs, block->num_active_local_vars);
}



static void init_bytecode(sno_State* state, sno_Compiler* cs) {
	sno_Bytecode* bytecode = sno_alloc_type(state, sno_Bytecode);
	bytecode->max_stack_needed = 64;
	bytecode->local_var_slots_needed = 0;
	cs->bytecode = bytecode;
}

static void init_function_compiler(sno_Tokenizer* ts, sno_Compiler* cs) {
	sno_State* state = ts->main_state;
	cs->ts = ts;
	init_bytecode(state, cs);
	cs->num_active_local_vars = 0;
	cs->max_active_local_vars = 0;
	cs->current_stack_idx = 0;
	cs->max_stack_used = 0;

	cs->parent = ts->cs;
	ts->cs = cs;
	sno_dynarray_init(state, &cs->local_vars, sizeof(sno_LocalVar), 4);
	sno_dynarray_init(state, &cs->number_constants, sizeof(sno_Number), 4);
	sno_dynarray_init(state, &cs->string_constants, sizeof(const sno_String*), 4);
	sno_dynarray_init(state, &cs->sub_functions, sizeof(sno_Bytecode*), 4);
	sno_dynarray_init(state, &cs->instructions, sizeof(sno_Instruction), 64);
	sno_dynarray_init(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), 64);
}

static void free_function_compiler(sno_Tokenizer* ts, sno_Compiler* cs) {
	sno_State* state = ts->main_state;
	sno_Bytecode* bc = cs->bytecode;
	//deactivate_local_variables(cs, 0);
	sno_assert(cs->current_block == NULL);

	bc->number_constants = sno_malloc(state, sizeof(sno_Number) * cs->number_constants.count);
	bc->num_number_constants = cs->number_constants.count;
	memcpy(bc->number_constants, cs->number_constants.buffer, sizeof(sno_Number) * cs->number_constants.count);

	bc->string_constants = sno_malloc(state, sizeof(sno_String*) * cs->string_constants.count);
	bc->num_string_constants = cs->string_constants.count;
	memcpy(bc->string_constants, cs->string_constants.buffer, sizeof(const sno_String*) * cs->string_constants.count);

	bc->sub_functions = sno_malloc(state, sizeof(sno_Bytecode*) * cs->sub_functions.count);
	bc->num_sub_functions = cs->sub_functions.count;
	memcpy(bc->sub_functions, cs->sub_functions.buffer, sizeof(sno_Bytecode*) * cs->sub_functions.count);

	//const sno_String* s = bc->string_constants[1];
	//printf("%.*s\n", s->len, s->str);

	bc->instructions = sno_malloc(state, sizeof(bc->instructions[0]) * cs->instructions.count);
	bc->instructions_size = cs->instructions.count;
	memcpy(bc->instructions, cs->instructions.buffer, sizeof(bc->instructions[0]) * cs->instructions.count);

	bc->instruction_source_code_offsets = sno_malloc(state, sizeof(bc->instruction_source_code_offsets[0]) * cs->instructions.count);
	memcpy(bc->instruction_source_code_offsets, cs->instruction_source_code_offsets.buffer, sizeof(bc->instruction_source_code_offsets[0]) * cs->instructions.count);

	bc->local_var_slots_needed = cs->max_active_local_vars;
	bc->local_vars = sno_malloc(state, sizeof(bc->local_vars[0]) * cs->local_vars.count);
	bc->num_local_vars = cs->local_vars.count;
	memcpy(bc->local_vars, cs->local_vars.buffer, sizeof(bc->local_vars[0]) * cs->local_vars.count);

	sno_dynarray_clear(state, &cs->number_constants);
	sno_dynarray_clear(state, &cs->string_constants);
	sno_dynarray_clear(state, &cs->sub_functions);
	sno_dynarray_clear(state, &cs->instructions);
	sno_dynarray_clear(state, &cs->local_vars);

	ts->cs = cs->parent;
}





static void parse_operand_primary(sno_Tokenizer* ts) {
	switch (ts->cur_token.type) {
	case sno_TK_NONE: {
		emit_instruction(ts, sno_I_LOAD_NONE);
		break;
	}
	case sno_TK_FALSE: {
		emit_instruction(ts, sno_I_LOAD_FALSE);
		break;
	}
	case sno_TK_TRUE: {
		emit_instruction(ts, sno_I_LOAD_TRUE);
		break;
	}
	case sno_TK_NUMBER: {
		emit_instruction_number(ts, ts->cur_token.info.u_number);
		break;
	}
	case sno_TK_STRING: {
		//code_instruction_1(ts, sno_I_LOAD_STRING, add_string_constant(ts->cs, ts->cur_token.info.u_string));
		break;
	}
	case sno_TK_LPAREN: {
		//sno_ReadNextToken(ts); // Skip '('
		//parse_expression(ts);
		//expect_token_and_skip(ts, sno_TK_RPAREN);
		return; // Skip reading the ')'
	}
	case sno_TK_LBRACKET: {
		//parse_array_constructor(ts);
		return; // Skip reading the ']'
	}
	case sno_TK_LBRACE: {
		//parse_table_constructor(ts);
		return; // Skip reading the '}'
	}
	case sno_TK_FUNCTION: {
		//sno_ReadNextToken(ts); // Skip 'function'
		//parse_function(ts);
		return; // Skip reading the '}'
	}
	case sno_TK_IDENTIFIER: {
		//identifier(ts, ts->cur_token.info.u_string);
		break;
	}
	case sno_TK_SELF: {
		if (!ts->cs->has_self_parameter) {
			//sno_ThrowSyntaxError(ts->main_state, ts->cur_token_linenum, "Function doesn't have the 'self' parameter");
		}
		//code_instruction_1(ts, sno_OP_GET_LOCAL, 0);
		break;
	}
	default: {
		//sno_ThrowSyntaxError(ts->main_state, ts->cur_token_linenum, "Invalid expression token '%s'", sno_token_strings[ts->cur_token.type]);
	}
	}
	sno_read_next_token(ts);
}

static void parse_operand(sno_Tokenizer* ts) {
	parse_operand_primary(ts);
	
}



static sno_UnOp get_unop(sno_TokenType tokentype) {
	switch (tokentype) {
	case sno_TK_SUB: return sno_UNOP_NEG;
	case sno_TK_BITFLIP: return sno_UNOP_BITFLIP;
	case sno_TK_LNOT: return sno_UNOP_LNOT;
	default: return sno_NOT_UNOP;
	}
}

static sno_BinOp get_binop(sno_TokenType tokentype) {
	switch (tokentype) {
	case sno_TK_ADD: return sno_BINOP_ADD;
	case sno_TK_SUB: return sno_BINOP_SUB;
	case sno_TK_MUL: return sno_BINOP_MUL;
	case sno_TK_DIV: return sno_BINOP_DIV;
	case sno_TK_IDIV: return sno_BINOP_IDIV;
	case sno_TK_MOD: return sno_BINOP_MOD;
	case sno_TK_POW: return sno_BINOP_POW;
	case sno_TK_BAND: return sno_BINOP_BAND;
	case sno_TK_BOR: return sno_BINOP_BOR;
	case sno_TK_BXOR: return sno_BINOP_BXOR;
	case sno_TK_SHL: return sno_BINOP_SHL;
	case sno_TK_SHR: return sno_BINOP_SHR;
	case sno_TK_EQ: return sno_BINOP_EQ;
	case sno_TK_NEQ: return sno_BINOP_NEQ;
	case sno_TK_LT: return sno_BINOP_LT;
	case sno_TK_GT: return sno_BINOP_GT;
	case sno_TK_LE: return sno_BINOP_LE;
	case sno_TK_GE: return sno_BINOP_GE;
	case sno_TK_LAND: return sno_BINOP_LAND;
	case sno_TK_LOR: return sno_BINOP_LOR;
	default: return sno_NOT_BINOP;
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

static sno_BinOp parse_subexpression(sno_Tokenizer* ts, unsigned int precedence) {
	sno_UnOp unary_op = get_unop(ts->cur_token.type);
	if (unary_op != sno_NOT_UNOP) {
		// Unary op
		sno_read_next_token(ts);
		parse_subexpression(ts, UNOP_PRECEDENCE);
		emit_instruction_1(ts, sno_I_UNOP, unary_op);
	} else {
		parse_operand(ts);
	}
	sno_BinOp binary_op = get_binop(ts->cur_token.type);
	while (binary_op != sno_NOT_BINOP && operator_precedence[binary_op].left > precedence) {
		sno_BinOp next_op;
		sno_read_next_token(ts);
		next_op = parse_subexpression(ts, operator_precedence[binary_op].right);
		emit_instruction_1(ts, sno_I_BINOP, binary_op);
		binary_op = next_op;
	}
	return binary_op;
}

static void parse_expression(sno_Tokenizer* ts) {
	parse_subexpression(ts, 0);

}



static void parse_expression_statement(sno_Tokenizer* ts) {
	parse_expression(ts);
}

/// @brief Parse a single statement, which should be the smallest completely separate pieces of code?
/// @param lexer 
/// @return sno_TRUE if this statement must be the last in a block,
/// because if it isn't, there would be unreachable code. sno_FALSE otherwise.
static sno_Bool parse_statement(sno_Tokenizer* ts) {
	printf("   Parsing statement starting with token ");
	sno_print_token(&ts->cur_token, &ts->next_token);
	printf("\n");

	switch (ts->cur_token.type) {
	case sno_TK_IF: {
		//parse_if_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_FOR: {
		//parse_for_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_WHILE: {
		//parse_while_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_BREAK: {
		//parse_break_statement(ts);
		return sno_TRUE;
	}
	case sno_TK_CONTINUE: {
		//parse_continue_statement(ts);
		return sno_TRUE;
	}
	case sno_TK_RETURN: {
		//parse_return_statement(ts);
		return sno_TRUE;
	}
	case sno_TK_VAR: case sno_TK_CONST: {
		//parse_declaration_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_FUNCTION: {
		//parse_function_statement(ts);
		return sno_FALSE;
	}
	default: {
		parse_expression_statement(ts);
		return sno_FALSE;
	}
	}
}

static void parse_block(sno_Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global_scope) {
	printf("Parsing block starting with token ");
	sno_print_token(&ts->cur_token, &ts->next_token);
	printf("\n");
	sno_assert_msg(!(is_loop && is_global_scope), "Global scope can't be a loop");

	sno_Block block;
	enter_block(ts->cs, &block, is_loop, is_global_scope);
	// 'return', 'break' and 'continue' statements must be at the end of a block
	sno_Bool is_last = sno_FALSE;
	while (!is_last) {
		sno_TokenType token = ts->cur_token.type;
		if (token == sno_TK_EOF || token == sno_TK_RBRACE) {
			// End of block
			break;
		}
		is_last = parse_statement(ts);
	}
	exit_block(ts->cs);
}



static sno_Bytecode* parse_source_code(sno_Tokenizer* ts) {
	sno_read_initial_tokens(ts);

	sno_Compiler cs = { 0 };
	init_function_compiler(ts, &cs);

	parse_block(ts, sno_FALSE, sno_TRUE);
	if (ts->cur_token.type != sno_TK_EOF) {
		sno_throw_syntax_error_at_token(
			ts,
			&ts->cur_token,
			"Global scope ended early here"
		);
	}
	emit_instruction(ts, sno_I_LOAD_NONE);
	emit_instruction(ts, sno_I_RETURN);

	free_function_compiler(ts, &cs);

#ifdef sno_DEBUG
	sno_print_bytecode(cs.bytecode);
#endif

	return cs.bytecode;
}



struct sno_Bytecode* sno_parse_source_code(
	sno_State* state,
	const sno_String* name,
	const sno_String* source_code
) {
	sno_assert_ptr(state);
	sno_assert_ptr(source_code);

	sno_Bool success = sno_TRUE;
	sno_Bytecode* bytecode = NULL;
	sno_ExceptionJump exception_jump;
	exception_jump.prev = state->exception_jump;
	state->exception_jump = &exception_jump;
	if (setjmp(exception_jump.buf) == 0) {
		sno_Tokenizer ts = { 0 };
		ts.main_state = state;
		ts.source_code_name = name;
		ts.source_code = source_code;
		ts.source_code_end = sno_string_chars(source_code) + source_code->length;
		ts.cur_char = sno_string_chars(source_code);
		ts.token_start = sno_string_chars(source_code);
		ts.line = 1;
		ts.column = 1;
		ts.cs = NULL;

		bytecode = parse_source_code(&ts);
	} else {
		fprintf(
			stderr,
			"%.*s\n",
			(unsigned int)state->exception_msg->length,
			sno_string_chars(state->exception_msg)
		);
		success = sno_FALSE;
	}

	//fputs(sno_ANSI_GREEN "Parsing success!" sno_ANSI_NORMAL "\n", stdout);

	//sno_Function* function = sno_CreateFunction(bytecode);
	//sno_PushFunction(state, function);

	state->exception_jump = state->exception_jump->prev;
	return sno_TRUE;
}
