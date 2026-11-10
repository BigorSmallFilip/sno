#include "sno_vm.h"

#include "sno_compiler.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>



const OpCodeInfo opcode_info[NUM_OPCODES] = {
	{ 0, 1, "NONE" },
	{ 0, 1, "FALSE" },
	{ 0, 1, "TRUE" },
	{ 0, 2, "NUMBER_I8" },
	{ 0, 3, "NUMBER" },
	{ 0, 3, "STRING" },
	{ 1, 2, "INTERPOLATE_STRING" },
	{ 0, 3, "BYTECODE" },
	{ 1, 2, "NEW_LINALG" },
	{ 1, 1, "NEW_ARRAY" },
	{ 1, 2, "CONCAT_ARRAY" },
	{ 1, 1, "NEW_TABLE" },
	{ 1, 2, "CONCAT_TABLE" },
	{ 0, 2, "GET_LOCAL" },
	{ 0, 2, "SET_LOCAL" },
	{ 1, 3, "GET_GLOBAL" },
	{ 1, 3, "SET_GLOBAL" },
	{ 1, 3, "GET_FIELD" },
	{ 1, 3, "SET_FIELD" },
	{ 1, 1, "GET_INDEX" },
	{ 1, 1, "SET_INDEX" },
	{ 1, 3, "SET_NEW_GLOBAL" },
	{ 1, 3, "GET_METHOD" },
	{ 0, 1, "POP" },
	{ 0, 1, "COPY_1" },
	{ 0, 1, "COPY_2" },
	{ 0, 1, "TO_BOOL" },
	{ 0, 1, "TO_BOOL_LNOT" },
	{ 1, 1, "NEG" },
	{ 1, 1, "BITFLIP" },
	{ 1, 1, "ADD" },
	{ 1, 1, "SUB" },
	{ 1, 1, "MUL" },
	{ 1, 1, "DIV" },
	{ 1, 1, "IDIV" },
	{ 1, 1, "MOD" },
	{ 1, 1, "POW" },
	{ 1, 1, "BAND" },
	{ 1, 1, "BOR" },
	{ 1, 1, "BXOR" },
	{ 1, 1, "SHL" },
	{ 1, 1, "SHR" },
	{ 1, 1, "LT" },
	{ 1, 1, "GT" },
	{ 1, 1, "LE" },
	{ 1, 1, "GE" },
	{ 0, 1, "EQ" },
	{ 0, 1, "NEQ" },
	{ 0, 3, "AND" },
	{ 0, 3, "OR" },
	{ 0, 3, "JMP_IF_FALSE" },
	{ 0, 3, "JMP" },
	{ 0, 3, "JMP_BACK" },
	{ 0, 3, "START_NUMERIC_FOR_LOOP" },
	{ 0, 3, "END_NUMERIC_FOR_LOOP" },
	{ 0, 3, "START_CONTAINER_FOR_LOOP" },
	{ 0, 3, "END_CONTAINER_FOR_LOOP" },
	{ 1, 2, "CALL" },
	{ 0, 2, "RETURN" },
};



static void print_instructions(const Bytecode* bytecode) {
	sno_assert_ptr(bytecode);
	PC pc = 0;
	uint32_t pos_i = 0;

	while (pc < bytecode->instructions_size) {
		OpCode opcode = bytecode->instructions[pc];
		sno_assert(opcode < NUM_OPCODES);
		const OpCodeInfo* info = &opcode_info[opcode];
		SourceCodePos pos = 0;
		if (info->has_pos) {
			pos = bytecode->instruction_positions[pos_i];
			sno_assert(pos != NO_POS);
			printf("%3u %3u >   %-18s ",
				(unsigned int)(pc),
				(unsigned int)pos,
				info->name
			);
			pos_i++;
		} else {
			pos = NO_POS;
			printf("%3u     >   %-18s ",
				(unsigned int)(pc),
				info->name
			);
		}
		uint16_t arg = 0;
		pc++;
		if (info->length == 2) {
			arg = bytecode->instructions[pc++];
		} else if (info->length == 3) {
			arg = bytecode->instructions[pc++];
			arg |= bytecode->instructions[pc++] << 8;
		}
		switch (opcode) {
		case OP_CONCAT_ARRAY:
		case OP_CONCAT_TABLE:
		case OP_NUMBER_I8: {
			printf("%i", (int)((int8_t)arg));
		} break;
		case OP_NUMBER: {
			sno_assert(arg < bytecode->num_number_constants);
			printf("%g", bytecode->number_constants[arg]);
		} break;
		case OP_STRING: {
			sno_assert(arg < bytecode->num_string_constants);
			IString* string = bytecode->string_constants[arg];
			printf("\"%.*s\"", (unsigned int)string->length, istring_chars(string));
		} break;
		case OP_INTERPOLATE_STRING:
		case OP_GET_LOCAL:
		case OP_SET_LOCAL: {
			printf("%i", (int)arg);
		} break;
		case OP_SET_GLOBAL:
		case OP_GET_GLOBAL:
		case OP_GET_FIELD:
		case OP_SET_FIELD:
		case OP_SET_NEW_GLOBAL:
		case OP_GET_METHOD: {
			sno_assert(arg < bytecode->num_string_constants);
			IString* name = bytecode->string_constants[arg];
			printf("%.*s", (unsigned int)name->length, istring_chars(name));
		} break;
		case OP_JMP_IF_FALSE:
		case OP_JMP:
		case OP_START_NUMERIC_FOR_LOOP:
		case OP_START_CONTAINER_FOR_LOOP:
		case OP_AND:
		case OP_OR: {
			printf("to %u", pc + arg);
		} break;
		case OP_END_NUMERIC_FOR_LOOP:
		case OP_END_CONTAINER_FOR_LOOP:
		case OP_JMP_BACK: {
			printf("to %u", pc - arg);
		} break;
		case OP_CALL: {
			sno_assert(arg != 0xFF);
			printf("argc = %i, retc = %i", (int)arg & 0xF, (int)arg >> 4);
		} break;
		case OP_RETURN: {	
			printf("retc = %i", (int)arg);
		} break;
		}
		putchar('\n');

#if 0
		if (pos != NO_POS) {
			char buffer[sno_STACK_BUFFER_LENGTH];
			size_t length = sprint_source_code_context(
				buffer,
				sno_STACK_BUFFER_LENGTH - 1,
				bytecode->source_code,
				pos
			);
			printf("%.*s\n" sno_ANSI_NORMAL, (unsigned int)length, buffer);
		}
#endif
	}
}

void print_bytecode(const Bytecode* bytecode) {
	if (!bytecode) {
		printf("No bytecode\n");
		return;
	}
	printf(
		"Bytecode for %.*s\n",
		(unsigned int)bytecode->name->length,
		istring_chars(bytecode->name)
	);
	printf(
		"In \"%.*s\"\n",
		(unsigned int)bytecode->source_code_name->length,
		istring_chars(bytecode->source_code_name)
	);
	print_instructions(bytecode);
	/*for (PC i = 0; i < bytecode->instructions_size;) {
		i = print_instructions(bytecode, i);
#if 0
		SourceCodePos pos = bytecode->instruction_positions[i];
		if (pos != NO_POS) {
			char buffer[sno_STACK_BUFFER_LENGTH];
			size_t length = sprint_source_code_context(
				buffer,
				sno_STACK_BUFFER_LENGTH - 1,
				bytecode->source_code,
				pos
			);
			printf("%.*s\n\n\n" sno_ANSI_NORMAL, (unsigned int)length, buffer);
		} else {
			printf("\n\n");
		}
#endif
	}*/
}



static SourceCodePos find_pos(
	Bytecode* bytecode,
	const uint8_t* pc
) {
	sno_assert_ptr(bytecode);
	sno_assert_ptr(pc);
	const uint8_t* search_pc = bytecode->instructions;
	if (search_pc == pc) {
		sno_assert(*pc < NUM_OPCODES);
		sno_assert(opcode_info[*pc].has_pos);
		sno_assert(bytecode->num_instruction_positions > 0);
		return bytecode->instruction_positions[0];
	}
	SourceCodePos* pos = &bytecode->instruction_positions[0];
	while (search_pc < bytecode->instructions + bytecode->instructions_size) {
		sno_assert(search_pc <= pc);
		OpCode op = *search_pc;
		sno_assert(op < NUM_OPCODES);
		const OpCodeInfo* info = &opcode_info[op];
		search_pc += info->length;
		if (search_pc == pc) {
			return *pos;
		}
		if (info->has_pos) {
			pos++;
			sno_assert(
				pos - bytecode->instruction_positions <
				bytecode->num_instruction_positions
			);
		}
	}
	return NO_POS;
}

static void vm_throw(
	sno_State* state,
	const uint8_t* pc,
	const char* const format,
	...
) {
	sno_assert_ptr(state);
	sno_assert_ptr(pc);
	sno_assert_ptr(format);
	sno_assert(state->call_stack.count > 0);
	const CallInfo* call_info = &state->call_stack.buffer[
		state->call_stack.count - 1
	];
	char msg[sno_STACK_BUFFER_LENGTH];
	size_t length = 0;
	length += sprint_source_code_context(
		msg + length,
		sno_STACK_BUFFER_LENGTH - length,
		call_info->function.bytecode->source_code,
		find_pos(call_info->function.bytecode, pc)
	);
	va_list args;
	va_start(args, format);
	length += (size_t)vsnprintf(
		msg + length,
		sno_STACK_BUFFER_LENGTH - length,
		format,
		args
	);
	va_end(args);
	state_throw(state, EXCEPTION_RUNTIME_ERROR, msg, length);
}

static void arith(sno_State* state, const uint8_t* pc, BinOp op) {
	sno_assert_ptr(state);
	sno_assert_ptr(pc);
	sno_assert_ptr(op >= 0 && op < NUM_BINOPS);
}

#define OP_ARITH(binop, name)										      \
	state->stack_top--;												      \
	if (state->stack_top[0].type == VT_NUMBER &&					      \
		state->stack_top[-1].type == VT_NUMBER						      \
		) {															      \
		state->stack_top[-1].u.number binop state->stack_top[0].u.number; \
	} else {														      \
		arith(state, pc, BINOP_##name);								      \
	}



static Value get_field(
	sno_State* state,
	const OpCode* pc,
	IString* name
) {
	sno_assert_ptr(state);
	sno_assert_ptr(pc);
	sno_assert_ptr(name);
	Value* container = &state->stack_top[-1];
	Value out;
	Value key;
	key.type = VT_STRING;
	key.u.string = name;
	if (container->type == VT_STRING) {
		if (name == state->count_builtin_string) {
			IString* string = container->u.string;
			Value v;
			set_number(v, (sno_Number)string->length);
			return v;
		}
		if (!table_get_key(state, state->string_prototype, key, &out)) {
			vm_throw(
				state, pc,
				"String has no field named %.*s",
				(unsigned int)name->length,
				istring_chars(name)
			);
		}
	} else if (container->type == VT_ARRAY) {
		if (name == state->count_builtin_string) {
			Array* arr = container->u.arr;
			Value v;
			set_number(v, (sno_Number)arr->values.count);
			return v;
		}
		if (!table_get_key(state, state->array_prototype, key, &out)) {
			vm_throw(
				state, pc,
				"Array has no field named %.*s",
				(unsigned int)name->length,
				istring_chars(name)
			);
		}
	} else if (container->type == VT_TABLE) {
		Table* table = container->u.table;
		Table* prototype = table->prototype;
		if (!prototype) {
			if (name == state->count_builtin_string) {
				Value v;
				set_number(v, (sno_Number)table->count);
				return v;
			}
			prototype = state->table_prototype;
		}
		if (!table_get_key(state, prototype, key, &out)) {
			if (!table_get_key(state, table, key, &out)) {
				vm_throw(
					state, pc,
					"Table has no field named %.*s",
					(unsigned int)name->length,
					istring_chars(name)
				);
			}
		}
	} else {
		vm_throw(
			state, pc,
			"Cannot get fields on %s",
			get_value_type_noun_phrase(container->type)
		);
	}
	return out;
}



static void set_field(
	sno_State* state,
	const OpCode* pc,
	IString* name
) {
	sno_assert_ptr(state);
	sno_assert_ptr(pc);
	sno_assert_ptr(name);
	Value* container = &state->stack_top[-2];
	Value* value = &state->stack_top[-1];
	Value key;
	key.type = VT_STRING;
	key.u.string = name;
	if (container->type == VT_TABLE) {
		Table* table = container->u.table;
		Table* prototype = table->prototype;
		if (!prototype) {
			if (name == state->count_builtin_string) {
				vm_throw(state, pc, "Cannot set the count of a table");
			}
			prototype = state->table_prototype;
		}
		if (!table_set_key(state, prototype, key, *value)) {
			if (!table_set_key(state, table, key, *value)) {
				vm_throw(
					state, pc,
					"Table has no field named %.*s",
					(unsigned int)name->length,
					istring_chars(name)
				);
			}
		}
	} else {
		vm_throw(
			state, pc,
			"Cannot set fields on %s",
			get_value_type_noun_phrase(container->type)
		);
	}
}



static size_t valid_int_index(
	sno_State* state,
	const OpCode* pc,
	size_t max,
	Value* index
) {
	if (index->type != VT_NUMBER) {
		vm_throw(
			state, pc, "Expected an integer index, but found %s",
			get_value_type_noun_phrase(index->type)
		);
	}
	sno_Number n = index->u.number;
	if (!sno_number_is_valid_i64(n)) {
		vm_throw(state, pc, "Index %g isn't an integer", n);
	}
	if (n < 0) {
		vm_throw(state, pc, "Index %g can't be negative", n);
	}
	size_t i = (size_t)n;
	if (i >= max) {
		vm_throw(state, pc, "Index %llu is out of bounds", i);
	}
	return i;
}

static Value get_index(
	sno_State* state,
	const OpCode* pc,
	Value* container,
	Value* key
) {
	sno_assert_ptr(state);
	sno_assert_ptr(pc);
	sno_assert_ptr(container);
	sno_assert_ptr(key);
	Value out;
	if (container->type == VT_STRING) {
		IString* string = container->u.string;
		size_t index = valid_int_index(state, pc, string->length, key);
		set_string(out, get_index_istring(state, string, index));
	} else if (container->type == VT_ARRAY) {
		Array* arr = container->u.arr;
		size_t index = valid_int_index(state, pc, arr->values.count, key);
		out = arr->values.buffer[index];
	} else if (container->type == VT_TABLE) {
		Table* table = container->u.table;
		if (table->prototype) {
			vm_throw(
				state, pc, "Cannot index into a table with a prototype"
			);
		}
		if (!table_get_key(state, table, *key, &out)) {
			set_none(out);
		}
	} else {
		vm_throw(
			state, pc,
			"Cannot index into %s",
			get_value_type_noun_phrase(container->type)
		);
	}
	return out;
}



static void set_index(
	sno_State* state,
	const OpCode* pc,
	Value* container,
	Value* key,
	Value* value
) {
	sno_assert_ptr(state);
	sno_assert_ptr(pc);
	sno_assert_ptr(container);
	sno_assert_ptr(key);
	sno_assert_ptr(value);
	if (container->type == VT_STRING) {
		IString* string = container->u.string;
		size_t index = valid_int_index(state, pc, string->length, key);
		if (value->type != VT_STRING) {
			vm_throw(
				state, pc,
				"Expected to set using a 1 character string, not %s",
				get_value_type_noun_phrase(value->type)
			);
		}
		IString* set_value = value->u.string;
		if (set_value->length != 1) {
			vm_throw(
				state, pc,
				"Expected to set using a 1 character string, "
				"but its length was %i",
				set_value->length
			);
		}
		set_index_istring(state, string, index, istring_chars(set_value)[0]);
	} else if (container->type == VT_ARRAY) {
		Array* arr = container->u.arr;
		size_t index = valid_int_index(state, pc, arr->values.count, key);
		arr->values.buffer[index] = *value;
	} else if (container->type == VT_TABLE) {
		Table* table = container->u.table;
		if (table->prototype) {
			vm_throw(
				state, pc, "Cannot index into a table with a prototype"
			);
		}
		(void)table_set_or_add_key(state, table, *key, *value);
	} else {
		vm_throw(
			state, pc,
			"Cannot index into %s",
			get_value_type_noun_phrase(container->type)
		);
	}
}



#define READ_U8() (*pc++)
#define READ_U16() ((uint16_t)((pc[1] << 8) | pc[0]))
#define READ_NUMBER_CONST \
	ConstID const_id = READ_U16(); \
	pc += 2; \
	sno_assert(const_id < bytecode->num_number_constants); \
	sno_Number number_const = bytecode->number_constants[const_id]
#define READ_STRING_CONST \
	ConstID const_id = READ_U16(); \
	pc += 2; \
	sno_assert(const_id < bytecode->num_string_constants); \
	IString* string_const = bytecode->string_constants[const_id]
#define READ_BYTECODE_CONST \
	ConstID const_id = READ_U16(); \
	pc += 2; \
	sno_assert(const_id < bytecode->num_bytecode_constants); \
	Bytecode* bytecode_const = bytecode->bytecode_constants[const_id]


#define DEBUG_EVERY_VM_INSTRUCTION
#ifdef DEBUG_EVERY_VM_INSTRUCTION
#define VM_DEBUG_OP \
    printf( \
        sno_ANSI_BLUE "pc=% 5u % 22s > sp=%u\n" sno_ANSI_NORMAL, \
        (unsigned int)(pc - bytecode->instructions), \
        opcode_info[op].name, \
        (unsigned int)(state->stack_top - state->stack) \
	)
#else
#define VM_DEBUG_OP
#endif


#define VM_LOOP \
	loop: {} \
    op = (OpCode)READ_U8(); \
    VM_DEBUG_OP; \
	switch (op)

#define VM_CASE(opcode) case opcode:
#define VM_DISPATCH() goto loop



uint8_t execute_bytecode(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_assert(argc <= MAX_EXPR_PER_STMT);
	sno_assert_msg(
		state->stack_top - state->stack >= argc + 2,
		"Too many arguments underflowing the stack"
	);
	Value* base = state->stack_top - (argc + 2) + 1;
	sno_assert(state->stack_base == base);
	sno_assert_msg(
		base[-1].type == VT_BYTECODE,
		"Tried to call something which wasn't a function"
	);
	Bytecode* bytecode = base[-1].u.bytecode;
	const uint8_t* pc = bytecode->instructions;
	OpCode op = (OpCode)*pc;
	Value* locals = base;
	state->stack_top += bytecode->num_local_slots;

	VM_LOOP {
	VM_CASE(OP_NONE)
		state->stack_top->type = VT_NONE;
		state->stack_top++;
		VM_DISPATCH();
	VM_CASE(OP_FALSE)
		state->stack_top->type = VT_BOOL;
		state->stack_top->u.number = NUMBER_FALSE;
		state->stack_top++;
		VM_DISPATCH();
	VM_CASE(OP_TRUE)
		state->stack_top->type = VT_BOOL;
		state->stack_top->u.number = NUMBER_TRUE;
		state->stack_top++;
		VM_DISPATCH();
	VM_CASE(OP_NUMBER_I8)
		state->stack_top->type = VT_NUMBER;
		state->stack_top->u.number = (sno_Number)(int8_t)(READ_U8());
		state->stack_top++;
		VM_DISPATCH();
	VM_CASE(OP_NUMBER) {
		READ_NUMBER_CONST;
		state->stack_top->type = VT_NUMBER;
		state->stack_top->u.number = number_const;
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_STRING) {
		READ_STRING_CONST;
		state->stack_top->type = VT_STRING;
		state->stack_top->u.string = string_const;
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_INTERPOLATE_STRING) {
		uint8_t concats = READ_U8();
		sno_assert(concats > 0);
		state->stack_top -= concats;
		IString* result = interpolate_istring(
			state, state->stack_top, concats
		);
		set_string(state->stack_top[0], result);
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_BYTECODE) {
		ConstID const_id = READ_U16();
		pc += 2;
		sno_assert(const_id < bytecode->num_bytecode_constants);
		Bytecode* bytecode_const = bytecode->bytecode_constants[const_id];
		state->stack_top->type = VT_BYTECODE;
		state->stack_top->u.bytecode = bytecode_const;
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_NEW_LINALG) {
		Array* arr = create_array(state);
		sno_assert(arr);
		state->stack_top->type = VT_ARRAY;
		state->stack_top->u.arr = arr;
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_NEW_ARRAY) {
		Array* arr = create_array(state);
		sno_assert(arr);
		state->stack_top->type = VT_ARRAY;
		state->stack_top->u.arr = arr;
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_CONCAT_ARRAY) {
		uint8_t count = READ_U8();
		sno_assert(state->stack_top[-count - 1].type == VT_ARRAY);
		Array* arr = state->stack_top[-count - 1].u.arr;
		sno_assert(arr);
		Value* items = state->stack_top - count;
		array_push_n(state, arr, items, count);
		state->stack_top -= count;
		VM_DISPATCH();
	}
	VM_CASE(OP_NEW_TABLE) {
		Table* table = create_table(state);
		sno_assert(table);
		state->stack_top->type = VT_TABLE;
		state->stack_top->u.table = table;
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_CONCAT_TABLE) {
		uint8_t count = READ_U8();
		sno_assert(state->stack_top[-count * 2 - 1].type == VT_TABLE);
		Table* table = state->stack_top[-count * 2 - 1].u.table;
		sno_assert(table);
		Value* items = state->stack_top - count * 2;
		for (size_t i = 0; i < count; i++) {
			if (table_set_or_add_key(
				state,
				table,
				items[i * 2],
				items[i * 2 + 1]
			)) {
				// Key already existed
				vm_throw(
					state,
					pc,
					"Repeated key in table constructor"
				);
			}
		}
		state->stack_top -= count * 2;
		VM_DISPATCH();
	}
	VM_CASE(OP_GET_LOCAL)
		* state->stack_top = locals[READ_U8()];
		state->stack_top++;
		VM_DISPATCH();
	VM_CASE(OP_SET_LOCAL)
		state->stack_top--;
		locals[READ_U8()] = *state->stack_top;
		VM_DISPATCH();
	VM_CASE(OP_GET_GLOBAL) {
		READ_STRING_CONST;
		Value key;
		key.type = VT_STRING;
		key.u.string = string_const;
		if (!table_get_key(state, state->globals, key, state->stack_top)) {
			vm_throw(state, pc,
				"There is no global variable named \"%.*s\"",
				(unsigned int)string_const->length,
				istring_chars(string_const)
			);
		}
		state->stack_top++;
		VM_DISPATCH();
	}
	VM_CASE(OP_SET_GLOBAL) {
		READ_STRING_CONST;
		state->stack_top--;
		Value key;
		key.type = VT_STRING;
		key.u.string = string_const;
		if (!table_set_key(state, state->globals, key, *state->stack_top)) {
			vm_throw(state, pc,
				"There is no global variable named \"%.*s\"",
				(unsigned int)string_const->length,
				istring_chars(string_const)
			);
		}
		VM_DISPATCH();
	}
	VM_CASE(OP_GET_FIELD) {
		READ_STRING_CONST;
		state->stack_top[-1] = get_field(state, pc, string_const);
		VM_DISPATCH();
	}
	VM_CASE(OP_SET_FIELD) {
		READ_STRING_CONST;
		set_field(state, pc, string_const);
		VM_DISPATCH();
	}
	VM_CASE(OP_GET_INDEX) {
		Value* container = &state->stack_top[-2];
		Value* key = &state->stack_top[-1];
		state->stack_top--;
		state->stack_top[-1] = get_index(state, pc, container, key);
		VM_DISPATCH();
	}
	VM_CASE(OP_SET_INDEX) {
		Value* container = &state->stack_top[-3];
		Value* key = &state->stack_top[-2];
		Value* value = &state->stack_top[-1];
		state->stack_top -= 3;
		set_index(state, pc, container, key, value);
		VM_DISPATCH();
	}
	VM_CASE(OP_SET_NEW_GLOBAL) {
		READ_STRING_CONST;
		state->stack_top--;
		Value key;
		key.type = VT_STRING;
		key.u.string = string_const;
		if (table_set_or_add_key(state, state->globals, key, *state->stack_top)) {
			vm_throw(state, pc,
				"A global variable named \"%.*s\" already exists",
				(unsigned int)string_const->length,
				istring_chars(string_const)
			);
		}
		VM_DISPATCH();
	}
	VM_CASE(OP_GET_METHOD) {
		READ_STRING_CONST;
		Value self = state->stack_top[-1];
		state->stack_top[-1] = get_field(state, pc, string_const);
		state->stack_top++;
		state->stack_top[-1] = self;
		VM_DISPATCH();
	}

	VM_CASE(OP_POP)
		state->stack_top--;
		VM_DISPATCH();
	VM_CASE(OP_COPY_1)
		state->stack_top++;
		state->stack_top[-1] = state->stack_top[-2];
		VM_DISPATCH();
	VM_CASE(OP_COPY_2)
		state->stack_top += 2;
		state->stack_top[-1] = state->stack_top[-3];
		state->stack_top[-2] = state->stack_top[-4];
		VM_DISPATCH();
	VM_CASE(OP_TO_BOOL)
		state->stack_top[-1].u.number = value_to_bool(
			state->stack_top[-1]
		) ? NUMBER_TRUE : NUMBER_FALSE;
		state->stack_top[-1].type = VT_BOOL;
		VM_DISPATCH();
	VM_CASE(OP_TO_BOOL_LNOT)
		state->stack_top[-1].u.number = value_to_bool(
			state->stack_top[-1]
		) ? NUMBER_FALSE : NUMBER_TRUE;
		state->stack_top[-1].type = VT_BOOL;
		VM_DISPATCH();
	VM_CASE(OP_NEG)
		if (
			state->stack_top[-1].type == VT_NUMBER ||
			state->stack_top[-1].type == VT_BOOL
		) {
			set_number(state->stack_top[-1], -state->stack_top[-1].u.number);
		} else {
			vm_throw(state, pc,
				"Tried to negate %s",
				get_value_type_noun_phrase(state->stack_top[-1].type)
			);
		}
		VM_DISPATCH();
	VM_CASE(OP_BITFLIP)
		if (
			state->stack_top[-1].type == VT_NUMBER ||
			state->stack_top[-1].type == VT_BOOL
		) {
			set_number(
				state->stack_top[-1],
				(sno_Number)(~(int32_t)(state->stack_top[-1].u.number))
			);
		} else {
			vm_throw(state, pc,
				"Tried to negate %s",
				get_value_type_noun_phrase(state->stack_top[-1].type)
			);
		}
		VM_DISPATCH();

	VM_CASE(OP_ADD)  OP_ARITH(+=, ADD)  VM_DISPATCH();
	VM_CASE(OP_SUB)  OP_ARITH(-=, SUB)  VM_DISPATCH();
	VM_CASE(OP_MUL)  OP_ARITH(*=, MUL)  VM_DISPATCH();
	VM_CASE(OP_DIV)  OP_ARITH(/=, DIV)  VM_DISPATCH();
	VM_CASE(OP_IDIV)
		state->stack_top--;
		if (state->stack_top[0].type == VT_NUMBER &&
			state->stack_top[-1].type == VT_NUMBER
			) {
			state->stack_top[-1].u.number = (sno_Number)(
				(int32_t)(state->stack_top[-1].u.number) /
				(int32_t)(state->stack_top[0].u.number)
			);
		} else {
			arith(state, pc, BINOP_IDIV);
		}
		VM_DISPATCH();
	VM_CASE(OP_MOD)
		state->stack_top--;
		if (state->stack_top[0].type == VT_NUMBER &&
			state->stack_top[-1].type == VT_NUMBER
			) {
			state->stack_top[-1].u.number = sno_mod(
				state->stack_top[-1].u.number,
				state->stack_top[-0].u.number
			);
		} else {
			arith(state, pc, BINOP_MOD);
		}
		VM_DISPATCH();
	VM_CASE(OP_POW)
		state->stack_top--;
		if (state->stack_top[0].type == VT_NUMBER &&
			state->stack_top[-1].type == VT_NUMBER
			) {
			state->stack_top[-1].u.number = sno_pow(
				state->stack_top[-1].u.number,
				state->stack_top[-0].u.number
			);
		} else {
			arith(state, pc, BINOP_POW);
		}
		VM_DISPATCH();
	VM_CASE(OP_BAND) OP_ARITH(+=, BAND) VM_DISPATCH();
	VM_CASE(OP_BOR)  OP_ARITH(+=, BOR)  VM_DISPATCH();
	VM_CASE(OP_BXOR) OP_ARITH(+=, BXOR) VM_DISPATCH();
	VM_CASE(OP_SHL)  OP_ARITH(+=, SHL)  VM_DISPATCH();
	VM_CASE(OP_SHR)  OP_ARITH(+=, SHR)  VM_DISPATCH();

	VM_CASE(OP_LT) {
		state->stack_top--;
		Value lhs = state->stack_top[-1];
		Value rhs = state->stack_top[0];
		if (lhs.type != VT_NUMBER || rhs.type != VT_NUMBER) {
			
		}
		VM_DISPATCH();
	}

	VM_CASE(OP_EQ)
		state->stack_top--;
		set_bool(state->stack_top[-1], value_eq(
			state->stack_top[0],
			state->stack_top[-1]
		));
		VM_DISPATCH();
	VM_CASE(OP_NEQ)
		state->stack_top--;
		set_bool(state->stack_top[-1], !value_eq(
			state->stack_top[0],
			state->stack_top[-1]
		));
		VM_DISPATCH();
	VM_CASE(OP_AND)
	VM_CASE(OP_OR) {
		uint16_t jump = READ_U16();
		pc += 2;
		sno_Bool b = value_to_bool(state->stack_top[-1]);
		if (b == (op == OP_OR)) {
			set_bool(state->stack_top[-1], b);
			pc += jump;
		} else {
			state->stack_top--;
		}
		VM_DISPATCH();
	}
	VM_CASE(OP_JMP_IF_FALSE) {
		uint16_t jump = READ_U16();
		pc += 2;
		if (!value_to_bool(state->stack_top[-1])) {
			pc += jump;
		}
		state->stack_top--;
		VM_DISPATCH();
	}
	VM_CASE(OP_JMP) {
		uint16_t jump = READ_U16();
		pc += jump;
		VM_DISPATCH();
	}
	VM_CASE(OP_JMP_BACK) {
		uint16_t jump = READ_U16();
		pc -= jump;
		VM_DISPATCH();
	}

	VM_CASE(OP_START_NUMERIC_FOR_LOOP) {
		Value* start = state->stack_top - 3;
		Value* stop = state->stack_top - 2;
		Value* step = state->stack_top - 1;
		if (start->type != VT_NUMBER ||
			stop->type != VT_NUMBER ||
			step->type != VT_NUMBER
		) {
			vm_throw(
				state, pc,
				"For loop was given non number arguments"
			);
		}
		uint16_t jump = READ_U16();
		pc += 2;
		if (start->u.number < stop->u.number) {
			// Push the iter number for the STORE_LOCAL instruction after this
			set_number(*state->stack_top, start->u.number);
			state->stack_top++;
		} else {
			state->stack_top -= 3; // Discard the loop variables
			pc += jump; // Jump out
		}
		VM_DISPATCH();
	}
	VM_CASE(OP_END_NUMERIC_FOR_LOOP) {
		Value* start = state->stack_top - 3;
		Value* stop = state->stack_top - 2;
		Value* step = state->stack_top - 1;
		sno_assert(start->type == VT_NUMBER);
		sno_assert(stop->type == VT_NUMBER);
		sno_assert(step->type == VT_NUMBER);
		start->u.number += step->u.number;
		uint16_t jump = READ_U16();
		pc -= jump;
		VM_DISPATCH();
	}
	VM_CASE(OP_START_CONTAINER_FOR_LOOP) {
		state->stack_top++;
		Value* container = state->stack_top - 2;
		Value* index = state->stack_top - 1;
		index->type = VT_NUMBER; // Make sure the GC ignores this integer
		uint32_t jump = READ_U16();
		pc += 2;
		if (container->type == VT_ARRAY) {
			Array* arr = container->u.arr;
			index->u.i = 0;
			if (index->u.i < arr->values.count) {
				// Push the key and value for the STORE_LOCAL instructions after this
				state->stack_top += 2;
				state->stack_top[-2] = arr->values.buffer[index->u.i];
				set_number(state->stack_top[-1], (sno_Number)index->u.i);
			} else {
				pc += jump; // Exit out of the loop
				state->stack_top -= 2; // Remove loop frame
			}
		} else if (container->type == VT_TABLE) {
			Table* table = container->u.table;
			if (table->count == 0) {
				pc += jump; // Exit out of the loop
				state->stack_top -= 2; // Remove loop frame
			}
			TableEntry* first = table->entries;
			index->u.ptr = table->entries;
			while (first->key_type < 0) {
				first++;
				sno_assert_msg(
					first < table->entries + table->count,
					"The index should never overflow the array\n"
					"That would mean it doesn't find a single entry\n"
					"in a non-empty table"
				);
			}
			// Push the key and value for the STORE_LOCAL instructions after this
			state->stack_top += 2;
			state->stack_top[-2].type = first->value_type;
			state->stack_top[-2].u.i = first->value_union.i;
			state->stack_top[-1].type = first->key_type;
			state->stack_top[-1].u.i = first->key_union.i;
			index->u.ptr = first;
		}
		VM_DISPATCH();
	}
	VM_CASE(OP_END_CONTAINER_FOR_LOOP) {
		Value* container = state->stack_top - 2;
		Value* index = state->stack_top - 1;
		uint32_t jump = READ_U16();
		pc += 2;
		if (container->type == VT_ARRAY) {
			Array* arr = container->u.arr;
			index->u.i++;
			if (index->u.i < arr->values.count) {
				// Push the key and value for the STORE_LOCAL instructions after this
				state->stack_top += 2;
				state->stack_top[-2] = arr->values.buffer[index->u.i];
				set_number(state->stack_top[-1], (sno_Number)index->u.i);
			} else {
				// Exit out of the loop
				state->stack_top -= 2; // Remove loop frame
				goto end;
			}
		} else if (container->type == VT_TABLE) {
			Table* table = container->u.table;
			TableEntry* entry = index->u.ptr;
			do { // Find the next entry
				entry++;
				if (entry >= table->entries + table->capacity_mask + 1) {
					// Reached end of entries list
					// Exit out of the loop
					state->stack_top -= 2; // Remove loop frame
					goto end;
				}
			} while (entry->key_type < 0);
			// Push the key and value for the STORE_LOCAL instructions after this
			state->stack_top += 2;
			state->stack_top[-2].type = entry->value_type;
			state->stack_top[-2].u.i = entry->value_union.i;
			state->stack_top[-1].type = entry->key_type;
			state->stack_top[-1].u.i = entry->key_union.i;
			index->u.ptr = entry;
		}
		pc -= (uint16_t)jump;
	end:
		VM_DISPATCH();
	}

	VM_CASE(OP_CALL) {
		uint8_t arg = READ_U8();
		uint8_t call_argc = arg & 0x0F;
		uint8_t retc_expected = arg >> 4;
		//printf("Function call!!! argc = %i, retc = %i\n", (int)call_argc, (int)retc_expected);
		Value* func_value = state->stack_top - (call_argc + 2);
		if (
			func_value->type != VT_BYTECODE &&
			func_value->type != VT_FOREIGN_FUNCTION
		) {
			vm_throw(state, pc,
				"Tried to perform a call on %s",
				get_value_type_noun_phrase(func_value->type)
			);
		}
		sno_call(state, call_argc, retc_expected);
		VM_DISPATCH();
	}
	VM_CASE(OP_RETURN) {
		return READ_U8();
		//READ_U8();
		//break;
	}
		
	default:
		// Invalid instruction
		sno_assert(op != 0xFF);
		sno_unreachable;
		break;
	}
#ifdef sno_DEBUG
	sno_unreachable;
	return 0;
#endif
}
