#include "sno_parser.h"

#include "sno_state.h"
#include "sno_vm.h"
#include <string.h>



//#define DEBUG_PRINT_PARSER



static uint8_t add_number_constant(sno_Compiler* cs, sno_Number number) {
	sno_State* state = cs->ts->main_state;
	if (cs->number_constants.count > sno_MAX_NUMBER_CONSTANTS) {
		sno_throw_syntax_error_at_cur_token(
			cs->ts,
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

static uint8_t add_string_constant(sno_Compiler* cs, const sno_IString* string) {
	sno_State* state = cs->ts->main_state;
	if (cs->string_constants.count > sno_MAX_NUMBER_CONSTANTS) {
		sno_throw_syntax_error_at_cur_token(
			cs->ts,
			"There are too many strings in this function"
		);
	}
	for (size_t i = 0; i < cs->string_constants.count; i++) {
		const sno_IString* c = (sno_IString*)sno_dynarray_get_ptr(state, &cs->string_constants, i);
		if (c == string) {
			return i;
		}
	}
	sno_ConstID const_id = (sno_ConstID)cs->string_constants.count;
	sno_dynarray_push_back_ptr(state, &cs->string_constants, string);
	return const_id;
}

static uint8_t add_sub_function(sno_Compiler* cs, sno_Bytecode* bytecode) {
	sno_assert(cs->sub_functions.count < 256);
	uint8_t sub_function_index = cs->sub_functions.count;
	sno_dynarray_push_back_ptr(cs->ts->main_state, &cs->sub_functions, bytecode);
	return sub_function_index;
}



#define get_instruction_buffer(cs) ((sno_Instruction*)((cs)->instructions.buffer))
#define get_opcode(i) ((i) & 0xFF)

#define set_call_num_returns(i, n) (((i) & (~(0x0F << 12))) | ((n) << (12)))
#define set_jump_dst(ts, from, to) get_instruction_buffer((ts)->cs)[from] |= (((int8_t)((to) - (from) - 1)) << 8)

static sno_inline sno_Instruction get_last_instruction(const sno_Compiler* cs) {
	sno_assert_ptr(cs);
	sno_assert(cs->instructions.count > 0);
	const sno_Instruction* instructions = get_instruction_buffer(cs);
	sno_Instruction last_instruction = instructions[cs->instructions.count - 1];
	return last_instruction;
}

static sno_inline sno_Instruction* get_ptr_to_last_instruction(sno_Compiler* cs) {
	sno_assert_ptr(cs);
	sno_assert(cs->instructions.count > 0);
	sno_Instruction* instructions = get_instruction_buffer(cs);
	return &instructions[cs->instructions.count - 1];
}

static sno_inline sno_Bool check_last_expression_is_valid_lhs(
	sno_Tokenizer* ts,
	uint32_t pos_start,
	uint32_t pos_end
) {
	sno_Instruction op = get_opcode(get_last_instruction(ts->cs));
	if (op == sno_I_GET_LOCAL ||
		op == sno_I_GET_GLOBAL
	) {
		return sno_FALSE;
	}
	if (op == sno_I_GET_FIELD ||
		op == sno_I_GET_INDEX
	) {
		return sno_TRUE;
	}
	sno_throw_syntax_error_open_close(
		ts,
		pos_start,
		pos_end,
		"You cannot assign a value to this" // TODO: Work on this error msg
	);
}

static uint32_t emit_instruction(sno_Tokenizer* ts, sno_Instruction i) {
	sno_State* state = ts->main_state;
	sno_Compiler* cs = ts->cs;
	sno_dynarray_push_back(state, &cs->instructions, sizeof(sno_Instruction), &i);
	size_t source_code_offset = 0;
	sno_dynarray_push_back(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), &source_code_offset);
	return cs->instructions.count - 1;
}

static uint32_t emit_instruction_at(sno_Tokenizer* ts, sno_Instruction i, uint32_t pos) {
	sno_State* state = ts->main_state;
	sno_Compiler* cs = ts->cs;
	sno_dynarray_push_back(state, &cs->instructions, sizeof(sno_Instruction), &i);
	sno_dynarray_push_back(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), &pos);
	return cs->instructions.count - 1;
}

static uint32_t emit_instruction_1(sno_Tokenizer* ts, uint8_t op, uint8_t arg) {
	sno_State* state = ts->main_state;
	sno_Compiler* cs = ts->cs;
	sno_Instruction i = op | (arg << 8);
	sno_dynarray_push_back(state, &cs->instructions, sizeof(sno_Instruction), &i);
	size_t source_code_offset = 0;
	sno_dynarray_push_back(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), &source_code_offset);
	return cs->instructions.count - 1;
}

static uint32_t emit_instruction_1_at(sno_Tokenizer* ts, uint8_t op, uint8_t arg, uint32_t pos) {
	sno_State* state = ts->main_state;
	sno_Compiler* cs = ts->cs;
	sno_Instruction i = op | (arg << 8);
	sno_dynarray_push_back(state, &cs->instructions, sizeof(sno_Instruction), &i);
	sno_dynarray_push_back(state, &cs->instruction_source_code_offsets, sizeof(uint32_t), &pos);
	return cs->instructions.count - 1;
}

static uint32_t emit_instruction_number(sno_Tokenizer* ts, sno_Number number) {
	return emit_instruction_1(ts, sno_I_LOAD_NUMBER, add_number_constant(ts->cs, number));
}

static uint32_t emit_instruction_string(sno_Tokenizer* ts, const sno_IString* string) {
	return emit_instruction_1(ts, sno_I_LOAD_STRING, add_string_constant(ts->cs, string));
}



static sno_LocalID register_local_variable(sno_Tokenizer* ts, const sno_IString* name) {
	sno_LocalVar local;
	local.name = name;
	local.slot = ts->cs->num_active_local_var_slots;
	local.start_pc = ts->cs->instructions.count;
	local.end_pc = 0;
	sno_dynarray_push_back(ts->main_state, &ts->cs->local_vars, sizeof(sno_LocalVar), &local);
	return ts->cs->local_vars.count - 1;
}

static sno_LocalSlot try_declare_local_variable(sno_Tokenizer* ts, sno_Token token) {
	const sno_IString* name = token.info.u_string;
	for (sno_LocalSlot i = 1; i < ts->cs->num_active_local_var_slots; i++) {
		sno_LocalID local_id = ts->cs->active_local_vars[i];
		const sno_LocalVar* local = (sno_LocalVar*)sno_dynarray_get(
			ts->main_state,
			&ts->cs->local_vars,
			sizeof(sno_LocalVar),
			local_id
		);
		if (local->name == name) {
			sno_throw_syntax_error_at(
				ts,
				token.source_code_pos,
				"A local variable called '%.*s' already exists",
				name->length,
				sno_string_chars(name)
			);
		}
	}
	if (ts->cs->num_active_local_var_slots > sno_MAX_ACTIVE_LOCAL_VARS) {
		sno_throw_syntax_error_at(
			ts,
			token.source_code_pos,
			"This function has too many local variables."
		);
	}
	sno_LocalID local_id = register_local_variable(ts, name);
	ts->cs->active_local_vars[ts->cs->num_active_local_var_slots] = local_id;
	ts->cs->num_active_local_var_slots++;
	if (ts->cs->num_active_local_var_slots > ts->cs->max_active_local_var_slots) {
		ts->cs->max_active_local_var_slots = ts->cs->num_active_local_var_slots;
	}
	return ts->cs->num_active_local_var_slots - 1;
}

static void deactivate_local_variables(sno_Compiler* cs, sno_LocalSlot to_id) {
	sno_assert(to_id >= 1 && to_id <= sno_MAX_ACTIVE_LOCAL_VARS);
	for (int i = cs->num_active_local_var_slots - 1; i >= (int)to_id; i--) {
		sno_LocalVar* a = sno_dynarray_get(cs->ts->main_state, &cs->local_vars, sizeof(sno_LocalVar), cs->active_local_vars[i]);
		a->end_pc = cs->instructions.count;
	}
	cs->num_active_local_var_slots = to_id;
}

static int search_local_variable_in_function(sno_Compiler* cs, const sno_IString* name) {
	sno_assert(cs->num_active_local_var_slots < sno_MAX_ACTIVE_LOCAL_VARS);
	for (int i = cs->num_active_local_var_slots - 1; i >= 1; i--) {
		sno_LocalVar* local = sno_dynarray_get(cs->ts->main_state, &cs->local_vars, sizeof(sno_LocalVar), cs->active_local_vars[i]);
		if (local->name == name) {
			return i;
		}
	}
	return -1;
}

static sno_Bool recursive_search_local_variable(sno_Compiler* cs, const sno_IString* name) {
	if (cs->current_block->is_global) {
		// If you've reached the global scope then stop searching
		return sno_FALSE;
	}
	int id = search_local_variable_in_function(cs, name);
	if (id >= 0) {
		// Id 0 should always be the 'self' argument
		sno_assert(id >= 1 && id < sno_MAX_ACTIVE_LOCAL_VARS);
		emit_instruction_1(cs->ts, sno_I_GET_LOCAL, id);
		return sno_TRUE;
	} else {
		if (!cs->parent || !recursive_search_local_variable(cs->parent, name)) {
			return sno_FALSE;
		} else {
			// Upval found
			sno_not_implemented;
		}
	}
	sno_unreachable;
	return sno_FALSE;
}

static void identifier(sno_Tokenizer* ts, const sno_Token* name) {
	if (!recursive_search_local_variable(ts->cs, name->info.u_string)) {
		// Nothing found so treat it like a global
		emit_instruction_1_at(
			ts,
			sno_I_GET_GLOBAL,
			add_string_constant(ts->cs, name->info.u_string),
			name->source_code_pos
		);
	}
}



static void enter_block(sno_Compiler* cs, sno_Block* block, sno_Bool is_loop, sno_Bool is_global) {
	sno_assert(!(is_loop && is_global));
	block->is_loop = is_loop;
	block->is_global = is_global;
	block->num_active_local_vars = cs->num_active_local_var_slots;
	block->prev = cs->current_block;
	cs->current_block = block;
	cs->current_block_depth++;
	if (cs->current_block_depth > sno_MAX_BLOCK_DEPTH) {
		sno_throw_syntax_error_at_cur_token(
			cs->ts,
			"Function compiles too deeply. Consider breaking it up into multiple separate functions"
		);
	}
}

static void exit_block(sno_Compiler* cs) {
	sno_Block* block = cs->current_block; // The block to exit out of
	cs->current_block = block->prev;
	cs->current_block_depth--;
	deactivate_local_variables(cs, block->num_active_local_vars);
}



static void init_bytecode(sno_State* state, sno_Tokenizer* ts, sno_Compiler* cs) {
	sno_Bytecode* bytecode = sno_alloc_type(state, sno_Bytecode);
	bytecode->gc_mark = 0;
	bytecode->gc_type = sno_OT_BYTECODE;
	bytecode->gc_next = NULL;

	bytecode->max_stack_needed = 17;
	bytecode->local_var_slots = 0;
	bytecode->source_code = ts->source_code;
	bytecode->name = ts->source_code_name;
	cs->bytecode = bytecode;
}

static void init_function_compiler(sno_Tokenizer* ts, sno_Compiler* cs) {
	sno_State* state = ts->main_state;
	cs->ts = ts;
	init_bytecode(state, ts, cs);
	cs->num_active_local_var_slots = 1;
	cs->max_active_local_var_slots = 1;
	cs->current_stack_idx = 0;
	cs->max_stack_used = 0;

	cs->parent = ts->cs;
	ts->cs = cs;
	sno_dynarray_init(state, &cs->local_vars, sizeof(sno_LocalVar), 4);
	sno_dynarray_init(state, &cs->number_constants, sizeof(sno_Number), 4);
	sno_dynarray_init(state, &cs->string_constants, sizeof(const sno_IString*), 4);
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

	bc->string_constants = sno_malloc(state, sizeof(sno_IString*) * cs->string_constants.count);
	bc->num_string_constants = cs->string_constants.count;
	memcpy(bc->string_constants, cs->string_constants.buffer, sizeof(const sno_IString*) * cs->string_constants.count);

	bc->sub_functions = sno_malloc(state, sizeof(sno_Bytecode*) * cs->sub_functions.count);
	bc->num_sub_functions = cs->sub_functions.count;
	memcpy(bc->sub_functions, cs->sub_functions.buffer, sizeof(sno_Bytecode*) * cs->sub_functions.count);

	//const sno_String* s = bc->string_constants[1];
	//printf("%.*s\n", s->len, s->str);

	bc->instructions = sno_malloc(state, sizeof(bc->instructions[0]) * cs->instructions.count);
	bc->num_instructions = cs->instructions.count;
	memcpy(bc->instructions, cs->instructions.buffer, sizeof(bc->instructions[0]) * cs->instructions.count);

	bc->instruction_source_code_offsets = sno_malloc(state, sizeof(bc->instruction_source_code_offsets[0]) * cs->instructions.count);
	memcpy(bc->instruction_source_code_offsets, cs->instruction_source_code_offsets.buffer, sizeof(bc->instruction_source_code_offsets[0]) * cs->instructions.count);

	bc->local_var_slots = cs->max_active_local_var_slots;
	bc->local_vars = sno_malloc(state, sizeof(bc->local_vars[0]) * cs->local_vars.count);
	bc->num_local_vars = cs->local_vars.count;
	memcpy(bc->local_vars, cs->local_vars.buffer, sizeof(bc->local_vars[0]) * cs->local_vars.count);

	sno_dynarray_clear(state, &cs->number_constants, sizeof(sno_Number));
	sno_dynarray_clear(state, &cs->string_constants, sizeof(const sno_IString*));
	sno_dynarray_clear(state, &cs->sub_functions, sizeof(sno_Bytecode*));
	sno_dynarray_clear(state, &cs->instructions, sizeof(sno_Instruction));
	sno_dynarray_clear(state, &cs->local_vars, sizeof(sno_LocalVar));

	bc->gc_next = state->gc_list_start;
	bc->gc_type = sno_OT_BYTECODE;
	bc->gc_mark = 0;
	state->gc_list_start = (sno_GCObject*)bc;
	state->num_non_string_gc_objects++;

	ts->cs = cs->parent;
}





static void expect_token(sno_Tokenizer* ts, sno_TokenType token) {
	if (ts->token.type != token) {
		const char* const why = sno_token_strings[token];
		sno_throw_syntax_error_at_cur_token(ts, "Expected a '%s' here", why);
	}
}

static void expect_token_and_skip(sno_Tokenizer* ts, sno_TokenType token) {
	expect_token(ts, token);
	sno_read_next_token(ts);
}

static void check_closing_token(sno_Tokenizer* ts, sno_TokenType type) {

}

static sno_inline void skip_token(sno_Tokenizer* ts, sno_TokenType type) {
	sno_assert_msg(ts->token.type == type, "Skipped token was different from what was expected");
	sno_read_next_token(ts);
}





static void parse_expression(sno_Tokenizer* ts);
static void parse_function(sno_Tokenizer* ts);
static uint32_t parse_closed_expression_list(sno_Tokenizer* ts, sno_TokenType end_token);
static void parse_block(sno_Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global_scope);
static void parse_brace_block(sno_Tokenizer* ts, sno_Bool is_loop);



static void parse_interpolated_string(sno_Tokenizer* ts) {
	emit_instruction_string(ts, ts->token.info.u_string);
	skip_token(ts, sno_TK_INTERPOLATED_STRING);
	uint8_t num_concats = 1;
	while (1) {
		num_concats += 2;
		parse_expression(ts);
		if (ts->token.type != sno_TK_RPAREN) {
			sno_throw_syntax_error_at_cur_token(ts, "Invalid string interpolation expression");
		}
		ts->cur_char = sno_string_chars(ts->source_code) + ts->token.source_code_pos;
		sno_continue_interpolated_string(ts);
		sno_assert(
			ts->token.type == sno_TK_STRING ||
			ts->token.type == sno_TK_INTERPOLATED_STRING
		);
		emit_instruction_string(ts, ts->token.info.u_string);
		if (ts->token.type == sno_TK_STRING) {
			sno_read_next_token(ts);
			break;
		}
		sno_read_next_token(ts);
	}
	emit_instruction_1(ts, sno_I_INTERPOLATE_STRING, num_concats);
}

static void parse_linalg_constructor(sno_Tokenizer* ts) {
	uint32_t at = ts->token.source_code_pos;
	sno_TokenType type = ts->token.type - sno_TK_VEC2;
	uint8_t size = sno_min(type + 2, 4);
	sno_read_next_token(ts);
	expect_token_and_skip(ts, sno_TK_LPAREN);
	uint32_t num_values = parse_closed_expression_list(ts, sno_TK_RPAREN);
	if (num_values > size) {
		sno_throw_syntax_error_at(ts, at, "Too many values for this initializer");
	}
	sno_assert(num_values <= 16);
	uint8_t arg = type;
	arg |= num_values << 3;
	emit_instruction_1_at(ts, sno_I_NEW_LINALG, arg, at);
}

static void parse_array_constructor(sno_Tokenizer* ts) {
	uint32_t pos_open = ts->token.source_code_pos;
	emit_instruction_at(ts, sno_I_NEW_ARRAY, pos_open);
	skip_token(ts, sno_TK_LBRACKET);
	if (ts->token.type == sno_TK_RBRACKET) {
		skip_token(ts, sno_TK_RBRACKET);
		return;
	}
	uint32_t len = 1;
	parse_expression(ts);
	while (1) {
		if (ts->token.type == sno_TK_COMMA) {
			skip_token(ts, sno_TK_COMMA);
			if (ts->token.type == sno_TK_RBRACKET) {
				break;
			}
			parse_expression(ts);
			len++;
			if (len >= sno_MAX_STACK_CONSTRUCTOR_ARGS) {
				emit_instruction_1_at(ts, sno_I_CONCAT_ARRAY, len, pos_open);
				len = 0;
			}
		} else if (ts->token.type == sno_TK_RBRACKET) {
			break;
		} else {
			sno_throw_syntax_error_open_close(
				ts,
				pos_open,
				ts->token.source_code_pos,
				"Array is missing a closing ']'"
			);
		}

	}
	if (len > 0) {
		emit_instruction_1_at(ts, sno_I_CONCAT_ARRAY, len, ts->token.source_code_pos);
	}
	skip_token(ts, sno_TK_RBRACKET);
	return;
}

static void parse_key_value_pair(sno_Tokenizer* ts) {
	if (ts->token.type == sno_TK_FUNCTION) {
		skip_token(ts, sno_TK_FUNCTION);
		expect_token(ts, sno_TK_IDENTIFIER);
		emit_instruction_string(ts, ts->token.info.u_string);
		skip_token(ts, sno_TK_IDENTIFIER);
		parse_function(ts);
		return;
	} else if (ts->token.type == sno_TK_IDENTIFIER) {
		// String key
		const sno_Token identifier_token = ts->token;
		emit_instruction_string(ts, identifier_token.info.u_string);
		skip_token(ts, sno_TK_IDENTIFIER);
		if (
			ts->token.type == sno_TK_COMMA ||
			ts->token.type == sno_TK_TERMINATOR ||
			ts->token.type == sno_TK_RBRACE
		) {
			// Shorthand struct field
			// "key": key
			identifier(ts, &identifier_token);
			return;
		}
	} else {
		// Key is an expression
		parse_expression(ts);
	}
	expect_token_and_skip(ts, sno_TK_COLON);
	parse_expression(ts); // Value expression
}

static void parse_table_constructor(sno_Tokenizer* ts) {
	uint32_t pos_open = ts->token.source_code_pos;
	emit_instruction_at(ts, sno_I_NEW_TABLE, pos_open);
	skip_token(ts, sno_TK_LBRACE);
	if (ts->token.type == sno_TK_RBRACE) {
		skip_token(ts, sno_TK_RBRACE);
		return;
	}
	uint32_t len = 1;
	parse_key_value_pair(ts);
	while (1) {
		if (
			ts->token.type == sno_TK_COMMA ||
			ts->token.type == sno_TK_TERMINATOR
		) {
			sno_read_next_token(ts);
			if (ts->token.type == sno_TK_RBRACE) {
				break;
			}
			parse_key_value_pair(ts);
			len++;
			if (len >= sno_MAX_STACK_CONSTRUCTOR_ARGS / 2) {
				emit_instruction_1_at(ts, sno_I_CONCAT_TABLE, len, pos_open);
				len = 0;
			}
		} else if (ts->token.type == sno_TK_RBRACE) {
			break;
		} else {
			sno_throw_syntax_error_open_close(
				ts,
				pos_open,
				ts->token.source_code_pos,
				"Table is missing a closing '}'"
			);
		}

	}
	skip_token(ts, sno_TK_RBRACE);
	if (len > 0) {
		emit_instruction_1_at(ts, sno_I_CONCAT_TABLE, len, pos_open);
	}
	return;
}

static void parse_function_parameters(sno_Tokenizer* ts) {
	expect_token_and_skip(ts, sno_TK_LPAREN);
	int num_parameters = 1;
	if (ts->token.type == sno_TK_RPAREN) {
		skip_token(ts, sno_TK_RPAREN);
		return;
	}
	if (ts->token.type == sno_TK_COMMA) {
		sno_throw_syntax_error_at_cur_token(ts, "Expected a parameter");
	}
	if (ts->token.type == sno_TK_CONST) {
		skip_token(ts, sno_TK_CONST);
	}
	if (ts->token.type == sno_TK_SELF) {
		ts->cs->has_self_parameter = sno_TRUE;
		skip_token(ts, sno_TK_SELF);
	}
	while (1) {
		if (ts->token.type == sno_TK_COMMA) {
			skip_token(ts, sno_TK_COMMA);
		}
		if (ts->token.type == sno_TK_RPAREN) {
			break;
		}
		if (ts->token.type == sno_TK_CONST) {
			skip_token(ts, sno_TK_CONST);
		}
		if (ts->token.type != sno_TK_IDENTIFIER) {
			sno_throw_syntax_error_at_cur_token(ts, "Expected a parameter");
		}
		try_declare_local_variable(ts, ts->token);
		skip_token(ts, sno_TK_IDENTIFIER);
		num_parameters++;
	}
	sno_assert(ts->token.type == sno_TK_RPAREN);
	skip_token(ts, sno_TK_RPAREN);
}

static void parse_function(sno_Tokenizer* ts) {
	sno_Compiler cs = { 0 };
	init_function_compiler(ts, &cs);

	parse_function_parameters(ts);

	parse_brace_block(ts, sno_FALSE);

	if (get_opcode(get_last_instruction(ts->cs)) != sno_I_RETURN) {
		emit_instruction_1(ts, sno_I_RETURN, 0);
	}
	sno_Bytecode* bytecode = cs.bytecode;
	free_function_compiler(ts, &cs);

#ifdef DEBUG_PRINT_PARSER
	sno_print_bytecode(bytecode);
#endif

	uint8_t sub_function_index = add_sub_function(ts->cs, bytecode);
	emit_instruction_1(ts, sno_I_LOAD_FUNCTION, sub_function_index);
}

static void parse_operand_primary(sno_Tokenizer* ts) {
	switch (ts->token.type) {
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
		emit_instruction_number(ts, ts->token.info.u_number);
		break;
	}
	case sno_TK_STRING: {
		emit_instruction_string(ts, ts->token.info.u_string);
		break;
	}
	case sno_TK_INTERPOLATED_STRING: {
		parse_interpolated_string(ts);
		return;
	}
	case sno_TK_VEC2:
	case sno_TK_VEC3:
	case sno_TK_VEC4:
	case sno_TK_QUAT:
	case sno_TK_MAT2:
	case sno_TK_MAT3:
	case sno_TK_MAT4: {
		parse_linalg_constructor(ts);
		return;
	}
	case sno_TK_LPAREN: {
		uint32_t open_paren = ts->token.source_code_pos;
		skip_token(ts, sno_TK_LPAREN);
		parse_expression(ts);
		if (ts->token.type != sno_TK_RPAREN) {
			sno_throw_syntax_error_open_close(
				ts,
				open_paren,
				ts->token.source_code_pos,
				"This parenthesis doesn't close"
			);
		}
		skip_token(ts, sno_TK_RPAREN);
		return;
	}
	case sno_TK_LBRACKET: {
		parse_array_constructor(ts);
		return; // Skip reading the ']'
	}
	case sno_TK_LBRACE: {
		parse_table_constructor(ts);
		return; // Skip reading the '}'
	}
	case sno_TK_FUNCTION: {
		skip_token(ts, sno_TK_FUNCTION);
		parse_function(ts);
		return; // Skip reading the '}'
	}
	case sno_TK_IDENTIFIER: {
		identifier(ts, &ts->token);
		break;
	}
	case sno_TK_SELF: {
		if (ts->cs->is_global_scope) {
			sno_throw_syntax_error_at_cur_token(ts, "Global scope doesn't have the 'self' parameter");
		}
		if (!ts->cs->has_self_parameter) {
			sno_throw_syntax_error_at_cur_token(ts, "This function doesn't have the 'self' parameter");
		}
		emit_instruction_1(ts, sno_I_GET_LOCAL, 0);
		break;
	}
	default: {
		sno_throw_syntax_error_at_cur_token(ts, "This is an invalid operand"); // TODO: Improve this message
		//sno_ThrowSyntaxError(ts->main_state, ts->cur_token_linenum, "Invalid expression token '%s'", sno_token_strings[ts->cur_token.type]);
	}
	}
	sno_read_next_token(ts);
}

static void parse_operand(sno_Tokenizer* ts) {
	parse_operand_primary(ts);
	while (1) { // For repeated application such as list[1][2].field[3]
		switch (ts->token.type) {
		case sno_TK_LBRACKET: { // Index
			uint32_t lbracket_at = ts->token.source_code_pos;
			skip_token(ts, sno_TK_LBRACKET);
			parse_expression(ts);
			expect_token_and_skip(ts, sno_TK_RBRACKET);
			emit_instruction_at(ts, sno_I_GET_INDEX, lbracket_at);
			break;
		}
		case sno_TK_DOT: { // Field or method call
			uint32_t dot_at = ts->token.source_code_pos;
			skip_token(ts, sno_TK_DOT);
			expect_token(ts, sno_TK_IDENTIFIER);
			uint8_t name_const_id = add_string_constant(ts->cs, ts->token.info.u_string);
			sno_read_next_token(ts);
			if (ts->token.type == sno_TK_LPAREN) {
				emit_instruction_1_at(ts, sno_I_GET_METHOD , name_const_id, dot_at);
				uint32_t lparen_at = ts->token.source_code_pos;
				skip_token(ts, sno_TK_LPAREN);
				uint32_t num_args = parse_closed_expression_list(ts, sno_TK_RPAREN);
				sno_Instruction call = sno_I_CALL | (num_args << 8) | (1 << 12);
				emit_instruction_at(ts, call, lparen_at);
			} else {
				emit_instruction_1_at(ts, sno_I_GET_FIELD, name_const_id, dot_at);
			}
			break;
		}
		case sno_TK_LPAREN: { // Function call
			uint32_t lparen_at = ts->token.source_code_pos;
			skip_token(ts, sno_TK_LPAREN);
			emit_instruction(ts, sno_I_LOAD_NONE); // Self parameter = null
			uint32_t num_args = parse_closed_expression_list(ts, sno_TK_RPAREN);
			sno_Instruction call = sno_I_CALL | (num_args << 8) | (1 << 12);
			emit_instruction_at(ts, call, lparen_at);
			break;
		}
		default: {
			return;
		}
		}
	}
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

static sno_BinOp parse_subexpression(
	sno_Tokenizer* ts,
	unsigned int precedence
) {
	sno_UnOp unary_op = get_unop(ts->token.type);
	if (unary_op != sno_NOT_UNOP) {
		// Unary op
		sno_read_next_token(ts);
		parse_subexpression(ts, UNOP_PRECEDENCE);
		emit_instruction_1(ts, sno_I_UNOP, unary_op);
	} else {
		parse_operand(ts);
	}
	sno_BinOp binary_op = get_binop(ts->token.type);
	while (binary_op != sno_NOT_BINOP && operator_precedence[binary_op].left > precedence) {
		sno_BinOp next_op;
		uint32_t binary_op_at = ts->token.source_code_pos;
		sno_read_next_token(ts);
		if (binary_op == sno_BINOP_LAND || binary_op == sno_BINOP_LOR) {
			uint32_t jump_from = emit_instruction(
				ts,
				binary_op == sno_BINOP_LAND ? sno_I_AND : sno_I_OR
			);
			next_op = parse_subexpression(ts, operator_precedence[binary_op].right);
			uint32_t jump_to = emit_instruction(ts, sno_I_TO_BOOL) + 1;
			set_jump_dst(ts, jump_from, jump_to);
		} else {
			next_op = parse_subexpression(ts, operator_precedence[binary_op].right);
			emit_instruction_1_at(ts, sno_I_BINOP, binary_op, binary_op_at);
		}
		binary_op = next_op;
	}
	return binary_op;
}

static void parse_expression(sno_Tokenizer* ts) {
	parse_subexpression(ts, 0);
}

static uint32_t parse_closed_expression_list(sno_Tokenizer* ts, sno_TokenType end_token) {
	uint32_t pos_open = ts->prev_token.source_code_pos;
	if (ts->token.type == end_token) {
		skip_token(ts, end_token);
		return 0;
	}
	uint32_t len = 1;
	parse_expression(ts);
	while (1) {
		if (ts->token.type == sno_TK_COMMA) {
			skip_token(ts, sno_TK_COMMA);
			if (ts->token.type == end_token) {
				break;
			}
			parse_expression(ts);
			len++;
		} else if (ts->token.type == end_token) {
			break;
		} else {
			sno_throw_syntax_error_open_close(
				ts,
				pos_open,
				ts->token.source_code_pos,
				"Missing a closing '%s'",
				sno_token_strings[end_token]
			);
		}
		
	}
	skip_token(ts, end_token);
	return len;
}



static void parse_if_statement(sno_Tokenizer* ts) {
	skip_token(ts, sno_TK_IF);
	parse_expression(ts);
	uint32_t jump_from = emit_instruction(ts, sno_I_JUMP_IF_FALSE);
	parse_brace_block(ts, sno_FALSE);
	uint32_t jump_to = ts->cs->instructions.count;
	if (ts->token.type == sno_TK_ELSE) {
		jump_to++;
		skip_token(ts, sno_TK_ELSE);
		uint32_t else_jump_from = emit_instruction(ts, sno_I_JUMP);
		if (ts->token.type == sno_TK_IF) {
			parse_if_statement(ts);
		} else {
			// Parse else block
			parse_brace_block(ts, sno_FALSE);
		}
		uint32_t else_jump_to = ts->cs->instructions.count;
		set_jump_dst(ts, else_jump_from, else_jump_to);
	}
	set_jump_dst(ts, jump_from, jump_to);
}

static void parse_for_statement(sno_Tokenizer* ts) {
	sno_Bool is_numeric_for_loop = sno_TRUE;
	const sno_Token for_token = ts->token;
	skip_token(ts, sno_TK_FOR);
	expect_token(ts, sno_TK_IDENTIFIER);
	sno_Token iter1 = ts->token;
	skip_token(ts, sno_TK_IDENTIFIER);
	sno_Token iter2 = { 0 };
	if (ts->token.type == sno_TK_COMMA) {
		skip_token(ts, sno_TK_COMMA);
		expect_token(ts, sno_TK_IDENTIFIER);
		iter2 = ts->token;
		skip_token(ts, sno_TK_IDENTIFIER);
		is_numeric_for_loop = sno_FALSE;
	}
	// iter variables done
	sno_Bool inclusive = sno_FALSE;
	if (ts->token.type == sno_TK_ASSIGN) {
		// Numeric for loop
		if (iter2.type == sno_TK_IDENTIFIER) {
			sno_throw_syntax_error_at(
				ts,
				iter2.source_code_pos,
				"Cannot use two iterator variables in a numberic for loop"
			);
		}
		sno_assert(is_numeric_for_loop);
		skip_token(ts, sno_TK_ASSIGN);
		parse_expression(ts); // Start
		expect_token_and_skip(ts, sno_TK_COMMA);
		if (ts->token.type == sno_TK_ASSIGN) {
			skip_token(ts, sno_TK_ASSIGN);
			inclusive = sno_TRUE;
		}
		parse_expression(ts); // Stop
		if (ts->token.type == sno_TK_COMMA) {
			skip_token(ts, sno_TK_COMMA);
			parse_expression(ts); // Step
		} else {
			emit_instruction_number(ts, 1); // Step is 1 by default
		}
	} else if (ts->token.type == sno_TK_IN) {
		// Container for loop
		if (iter2.type != sno_TK_IDENTIFIER) {
			
		}
		is_numeric_for_loop = sno_FALSE;
		skip_token(ts, sno_TK_IN);
		parse_expression(ts); // Thing to iterate
	} else {
		sno_throw_syntax_error_at_cur_token(
			ts, "Expected either an '=' or 'in' here"
		);
	}

	int iter_local_id = try_declare_local_variable(ts, iter1);
	if (iter2.type != 0) {
		try_declare_local_variable(ts, iter2);
	}

	uint32_t start = emit_instruction_at(
		ts,
		is_numeric_for_loop ?
			sno_I_START_NUMERIC_FORLOOP :
			sno_I_START_CONTAINER_FORLOOP,
		for_token.source_code_pos
	);
	if (is_numeric_for_loop) {
		emit_instruction_1(ts, sno_I_SET_LOCAL, iter_local_id);
	} else {
		if (iter2.type != 0) {
			emit_instruction_1(ts, sno_I_SET_LOCAL, iter_local_id);
			emit_instruction_1(ts, sno_I_SET_LOCAL, iter_local_id + 1);
		} else {
			emit_instruction(ts, sno_I_POP);
			emit_instruction_1(ts, sno_I_SET_LOCAL, iter_local_id);
		}
	}
	parse_brace_block(ts, sno_TRUE);
	uint32_t end = emit_instruction_at(
		ts,
		is_numeric_for_loop ? 
			sno_I_END_NUMERIC_FORLOOP :
			sno_I_END_CONTAINER_FORLOOP,
		for_token.source_code_pos
	);

	set_jump_dst(ts, start, end + 1);
	set_jump_dst(ts, end, start + (is_numeric_for_loop ? 0 : 1));
	
	deactivate_local_variables(ts->cs, iter_local_id);
}

static void parse_while_statement(sno_Tokenizer* ts) {
	skip_token(ts, sno_TK_WHILE);
	uint32_t back_to = ts->cs->instructions.count;
	parse_expression(ts);
	uint32_t condition_jump_from = emit_instruction(ts, sno_I_JUMP_IF_FALSE);
	parse_brace_block(ts, sno_TRUE);
	uint32_t back_from = emit_instruction(ts, sno_I_JUMP);
	uint32_t condition_jump_to = back_from + 1;
	set_jump_dst(ts, back_from, back_to);
	set_jump_dst(ts, condition_jump_from, condition_jump_to);
}

static void parse_break_statement(sno_Tokenizer* ts) {
	skip_token(ts, sno_TK_BREAK);
	emit_instruction_1(ts, sno_I_JUMP, 69);
}

static void parse_continue_statement(sno_Tokenizer* ts) {
	skip_token(ts, sno_TK_CONTINUE);
	emit_instruction_1(ts, sno_I_JUMP, 69);
}

static void parse_return_statement(sno_Tokenizer* ts) {
	if (ts->cs->is_global_scope) {
		sno_throw_syntax_error_at_cur_token(ts, "Only functions can have return statements");
	}
	skip_token(ts, sno_TK_RETURN);
	uint8_t num_returns = 0;
	if (ts->token.type == sno_TK_TERMINATOR) {
		emit_instruction(ts, sno_I_LOAD_NONE);
	} else {
		while (1) {
			num_returns++;
			if (num_returns > sno_MAX_STACK_ARGS) {
				sno_throw_syntax_error_at_cur_token(
					ts,
					"Too many return values. The max is 14"
				);
			}
			parse_expression(ts);
			if (ts->token.type == sno_TK_TERMINATOR) {
				break;
			} else if (ts->token.type == sno_TK_COMMA) {
				skip_token(ts, sno_TK_COMMA);
			} else {
				sno_throw_syntax_error_at_cur_token(ts, "Nope. I didn't expect this lol");
			}
		}
	}
	emit_instruction_1(ts, sno_I_RETURN, num_returns);
}

static void parse_declaration_statement(sno_Tokenizer* ts) {
	sno_Bool is_const = ts->token.type == sno_TK_CONST;
	sno_read_next_token(ts);
	sno_Token name_tokens[16];
	uint8_t num_declarations = 0;
	sno_Bool no_assignment = sno_FALSE;
	while (1) {
		if (ts->token.type != sno_TK_IDENTIFIER) {
			sno_throw_syntax_error_at_cur_token(ts, "Expected a name for a local variable here");
		}
		name_tokens[num_declarations] = ts->token;
		num_declarations++;
		if (num_declarations > sno_MAX_STACK_ARGS) {
			sno_throw_syntax_error_at_cur_token(
				ts,
				"Too many declarations in one statement. The max is 14"
			);
		}
		skip_token(ts, sno_TK_IDENTIFIER);
		if (ts->token.type == sno_TK_TERMINATOR) {
			skip_token(ts, sno_TK_TERMINATOR);
			no_assignment = sno_TRUE;
			break;
		}
		if (ts->token.type != sno_TK_COMMA) {
			break;
		}
		skip_token(ts, sno_TK_COMMA);
	}
	if (no_assignment) {
		for (uint8_t i = 0; i < num_declarations; i++) {
			emit_instruction(ts, sno_I_LOAD_NONE);
		}
	} else {
		sno_assert(num_declarations >= 1);
		uint32_t assignment_token_pos = ts->token.source_code_pos;
		expect_token_and_skip(ts, sno_TK_ASSIGN);
		for (uint8_t i = 0;; i++) {
			if (i > num_declarations) {
				sno_throw_syntax_error_at(
					ts,
					assignment_token_pos,
					"There are %i item(s) on the left but %i item(s) on the right",
					num_declarations,
					i + 1
				);
			}
			parse_expression(ts);
			if (ts->token.type == sno_TK_TERMINATOR) {
				sno_Instruction* last_instruction = get_ptr_to_last_instruction(ts->cs);
				if (get_opcode(*last_instruction) == sno_I_CALL) {
					*last_instruction = set_call_num_returns(*last_instruction, num_declarations - i);
					break;
				} else if (i + 1 != num_declarations) {
					// Wrong number of declarations on either side
					sno_throw_syntax_error_at(
						ts,
						assignment_token_pos,
						"There are %i item(s) on the left but %i item(s) on the right",
						num_declarations,
						i + 1
					);
				} else {
					break;
				}
			}
			expect_token_and_skip(ts, sno_TK_COMMA);
		}
		sno_Instruction* instructions = get_instruction_buffer(ts->cs);
	}
	sno_assert(num_declarations >= 1 && num_declarations <= sno_MAX_STACK_ARGS);
	for (int8_t i = (int8_t)num_declarations - 1; i >= 0; i--) {
		sno_Token name_token = name_tokens[i];
		if (ts->cs->current_block->is_global) {
			emit_instruction_1_at(
				ts,
				sno_I_NEW_GLOBAL,
				add_string_constant(ts->cs, name_token.info.u_string),
				name_token.source_code_pos
			);
		} else {
			sno_LocalSlot local_slot = try_declare_local_variable(ts, name_token);
			emit_instruction_1(ts, sno_I_SET_LOCAL, local_slot);
		}
	}
}

static void parse_expression_statement(sno_Tokenizer* ts) {
	uint32_t first_stmt_token_pos = ts->token.source_code_pos;
	parse_expression(ts);
	sno_Instruction last_instruction = get_last_instruction(ts->cs);
	if (get_opcode(last_instruction) == sno_I_CALL) {
		*get_ptr_to_last_instruction(ts->cs) = set_call_num_returns(last_instruction, 0);
		// Make sure it was the last thing on the statement
		if (ts->token.type != sno_TK_TERMINATOR) {
			sno_throw_syntax_error_open_close(
				ts,
				first_stmt_token_pos,
				ts->token.source_code_pos,
				"This function call should be the only thing in this statement" // TODO: work on this
			);
		} else {
			return;
		}
	}
	// Assignment statement
	sno_Bool shuffling_required = sno_FALSE;
	shuffling_required = check_last_expression_is_valid_lhs(
		ts,
		first_stmt_token_pos,
		&ts->token.source_code_pos
	);
	if (sno_token_is_assignment(ts->token.type)) {
		sno_TokenType assignment_token = ts->token.type;
		uint32_t assignment_at = ts->token.source_code_pos;
		sno_read_next_token(ts); // Skip assignment token
		if (assignment_token == sno_TK_ASSIGN) {
			ts->cs->instructions.count--; // Remove the final get-instruction, convert to set later
		} else {
			// Binop assignment
			if (get_opcode(last_instruction) == sno_I_GET_INDEX) {
				ts->cs->instructions.count--; // Remove the final get-instruction, convert to set later
				emit_instruction_1(ts, sno_I_COPY, 2);
				emit_instruction(ts, last_instruction);
			}
		}
		parse_expression(ts);
		if (assignment_token != sno_TK_ASSIGN) {
			// Binop assignment
			emit_instruction_1_at(ts, sno_I_BINOP, assignment_token - sno_TK_ASSIGNADD, assignment_at);
		}
		emit_instruction(ts, last_instruction + 1);
		return;
	}
	if (ts->token.type == sno_TK_INC || ts->token.type == sno_TK_DEC) {
		sno_Bool is_inc = (ts->token.type == sno_TK_INC);
		emit_instruction_1_at(
			ts,
			sno_I_UNOP,
			is_inc ? sno_UNOP_INC : sno_UNOP_DEC,
			ts->token.source_code_pos
		);
		emit_instruction(ts, last_instruction + 1);
		sno_read_next_token(ts);
		return;
	}

	sno_Instruction deferred_instructions[sno_MAX_STACK_ARGS];
	// Defer the last instruction
	deferred_instructions[0] = last_instruction;
	ts->cs->instructions.count--;

	uint8_t num_lhs = 1;
	while (1) {
		if (ts->token.type == sno_TK_COMMA) {
			num_lhs++;
			if (num_lhs > sno_MAX_STACK_ARGS) {
				sno_throw_syntax_error_at_cur_token(
					ts,
					"Too many assignments in one statement. The max is 14"
				);
			}
			skip_token(ts, sno_TK_COMMA);
			uint32_t first_token_pos = ts->token.source_code_pos;
			parse_expression(ts);
			shuffling_required |= check_last_expression_is_valid_lhs(
				ts,
				first_token_pos,
				ts->token.source_code_pos
			);
			// Defer the last instruction
			deferred_instructions[num_lhs - 1] = get_last_instruction(ts->cs);
			ts->cs->instructions.count--;
			continue;
		} else {
			break;
		}
	}
	sno_assert(num_lhs < sno_MAX_STACK_ARGS);
	sno_assert(num_lhs >= 1);
	if (ts->token.type != sno_TK_ASSIGN) {
		if (sno_token_is_assignment(ts->token.type)) {
			// TODO: Better error message
		}
		sno_throw_syntax_error_at_cur_token(
			ts,
			"Expected an '=' here. "
			"All expression statements other than function calls must assign to something"
		);
	}
	uint32_t assignment_token_pos = ts->token.source_code_pos;
	skip_token(ts, sno_TK_ASSIGN);
	uint8_t num_rhs = 1;
	for (uint8_t i = 0; i < num_lhs; i++) {
		parse_expression(ts);
		sno_Bool stmt_end = sno_FALSE;
		if (ts->token.type == sno_TK_TERMINATOR) {
			stmt_end = sno_TRUE;
			sno_read_next_token(ts);
		}
		sno_Instruction* instructions = get_instruction_buffer(ts->cs);
		last_instruction = get_last_instruction(ts->cs);
		if (get_opcode(last_instruction) == sno_I_CALL) {
			uint8_t num_returns = 1;
			if (stmt_end) {
				// This is the last rhs expression
				// so call should get values for all remaining lhs
				num_returns = num_lhs - i;
			}
			instructions[ts->cs->instructions.count - 1] =
				set_call_num_returns(last_instruction, num_returns);
		}
		if (stmt_end) {
			break;
		}
		if (ts->token.type == sno_TK_COMMA) {
			skip_token(ts, sno_TK_COMMA);
			num_rhs++;
			if (num_rhs > sno_MAX_STACK_ARGS) {
				sno_throw_syntax_error_at_cur_token(
					ts,
					"Too many expressions in one statement. The max is 14"
				);
			}
			if (num_rhs > num_lhs) {
				sno_throw_syntax_error_at(
					ts,
					assignment_token_pos,
					"There are %i item(s) on the left but %i item(s) on the right",
					num_lhs,
					num_rhs
				);
			}
		}
	}
	if (get_opcode(last_instruction) != sno_I_CALL) {
		if (num_lhs != num_rhs) {
			sno_throw_syntax_error_at(
				ts,
				assignment_token_pos,
				"There are %i item(s) on the left but %i item(s) on the right",
				num_lhs,
				num_rhs
			);
		}
	}
	//emit_instruction_1(ts, sno_I_REV, num_lhs);
	if (shuffling_required) {
		emit_instruction_1(ts, sno_I_MULTI_ASSIGN_SHUFFLE, num_lhs);
	}
	for (int8_t i = num_lhs - 1; i >= 0; i--) {
		sno_Instruction deferred_instruction = deferred_instructions[i];
		emit_instruction(ts, deferred_instruction + 1);
	}
	if (ts->token.type == sno_TK_COMMA) {
		sno_throw_syntax_error_at_cur_token(ts, "More expressions than assignments here");
	}
}

static void parse_function_statement(sno_Tokenizer* ts) {
	if (!ts->cs->current_block->is_global) {
		sno_throw_syntax_error_at_cur_token(
			ts,
			"Functions can only be declared this way in the global scope"
		);
	}
	skip_token(ts, sno_TK_FUNCTION);
	uint32_t name_at = ts->token.source_code_pos;
	const sno_IString* name = ts->token.info.u_string;
	skip_token(ts, sno_TK_IDENTIFIER);
	parse_function(ts);
	emit_instruction_1_at(
		ts,
		sno_I_NEW_GLOBAL,
		add_string_constant(ts->cs, name),
		name_at
	);
}



/// @brief Parse a single statement, which should be the smallest completely separate pieces of code?
/// @param lexer 
/// @return sno_TRUE if this statement must be the last in a block,
/// because if it isn't, there would be unreachable code. sno_FALSE otherwise.
static sno_Bool parse_statement(sno_Tokenizer* ts) {
	//printf("   Parsing statement starting with token ");
	//sno_print_token(&ts->token);
	//printf("\n");

	switch (ts->token.type) {
	case sno_TK_IF: {
		parse_if_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_FOR: {
		parse_for_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_WHILE: {
		parse_while_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_BREAK: {
		parse_break_statement(ts);
		return sno_TRUE;
	}
	case sno_TK_CONTINUE: {
		parse_continue_statement(ts);
		return sno_TRUE;
	}
	case sno_TK_RETURN: {
		parse_return_statement(ts);
		return sno_TRUE;
	}
	case sno_TK_VAR: case sno_TK_CONST: {
		parse_declaration_statement(ts);
		return sno_FALSE;
	}
	case sno_TK_FUNCTION: {
		parse_function_statement(ts);
		return sno_FALSE;
	}
	default: {
		parse_expression_statement(ts);
		return sno_FALSE;
	}
	}
}

static void parse_block(sno_Tokenizer* ts, sno_Bool is_loop, sno_Bool is_global_scope) {
	//printf("Parsing block starting with token ");
	//sno_print_token(&ts->token);
	//printf("\n");
	sno_assert_msg(!(is_loop && is_global_scope), "Global scope can't be a loop");

	sno_Block block;
	enter_block(ts->cs, &block, is_loop, is_global_scope);
	// 'return', 'break' and 'continue' statements must be at the end of a block
	sno_Bool is_last = sno_FALSE;
	while (!is_last) {
		sno_TokenType token = ts->token.type;
		if (token == sno_TK_EOF || token == sno_TK_RBRACE) {
			// End of block
			break;
		}
		is_last = parse_statement(ts);
		if (
			ts->token.type != sno_TK_TERMINATOR
		) {
			sno_throw_syntax_error_at_cur_token(
				ts,
				"Statement didn't end properly"
			);
		}
		sno_read_next_token(ts);
	}
	exit_block(ts->cs);
}

static void parse_brace_block(sno_Tokenizer* ts, sno_Bool is_loop) {
	expect_token_and_skip(ts, sno_TK_LBRACE);
	parse_block(ts, is_loop, sno_FALSE);
	expect_token_and_skip(ts, sno_TK_RBRACE);
}



static sno_Bytecode* parse_source_code(sno_Tokenizer* ts) {
	sno_read_initial_token(ts);

	sno_Compiler cs = { 0 };
	init_function_compiler(ts, &cs);
	cs.is_global_scope = sno_TRUE;

	parse_block(ts, sno_FALSE, sno_TRUE);
	if (ts->token.type != sno_TK_EOF) {
		sno_unreachable;
		//sno_throw_syntax_error_at_cur_token(ts, "Global scope ended early here");
	}
	emit_instruction_1(ts, sno_I_RETURN, 0);

	free_function_compiler(ts, &cs);

#ifdef DEBUG_PRINT_PARSER
	sno_print_bytecode(cs.bytecode);
#endif

	return cs.bytecode;
}



struct sno_Bytecode* sno_parse_source_code(
	sno_State* state,
	const sno_IString* name,
	const sno_IString* source_code
) {
	sno_assert_ptr(state);
	sno_assert_ptr(name);
	sno_assert_ptr(source_code);

#ifdef DEBUG_PRINT_PARSER
	//sno_print_source_code(state, name, source_code);
#endif

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
		ts.cs = NULL;

		bytecode = parse_source_code(&ts);
	} else {
		success = sno_FALSE;
	}

	//fputs(sno_ANSI_GREEN "Parsing success!" sno_ANSI_NORMAL "\n", stdout);

	//sno_Function* function = sno_CreateFunction(bytecode);
	//sno_PushFunction(state, function);

	state->exception_jump = state->exception_jump->prev;
	return bytecode;
}
