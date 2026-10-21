#include "sno_vm.h"

#include "sno_compiler.h"
#include <stdio.h>
#include <stdarg.h>



const OpCodeInfo opcode_info[NUM_OPCODES] = {
	{ 0, 1, "NONE" },
	{ 0, 1, "FALSE" },
	{ 0, 1, "TRUE" },
	{ 0, 2, "NUMBER_I8" },
	{ 0, 3, "NUMBER" },
	{ 0, 3, "STRING" },
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
	sno_State* state,
	Bytecode* bytecode,
	const uint8_t* pc
) {
	sno_assert_ptr(state);
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
	sno_unreachable;
	return 0;
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
		find_pos(state, call_info->function.bytecode, pc)
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


#define VM_LOOP \
	loop: \
	switch (op = (OpCode)READ_U8())

#define VM_CASE(opcode) case opcode:
#define VM_DISPATCH() goto loop



uint8_t execute_bytecode(sno_State* state, uint8_t argc) {
	sno_assert_ptr(state);
	sno_assert(argc <= MAX_EXPR_PER_STMT);
	sno_assert_msg(
		state->stack_top - state->stack >= argc + 2,
		"Too many arguments underflowing the stack"
	);
	Value* base = state->stack_top - (argc + 2);
	sno_assert_msg(
		base->type == VT_BYTECODE,
		"Tried to call something which wasn't a function"
	);
	Bytecode* bytecode = base->u.bytecode;
	const uint8_t* pc = bytecode->instructions;
	OpCode op = (OpCode)*pc;
	Value* locals = base + 2;
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
	VM_CASE(OP_GET_LOCAL) {
		*state->stack_top = locals[READ_U8()];
		state->stack_top++;
		VM_DISPATCH();
	}
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
	VM_CASE(OP_CALL) {
		uint8_t arg = READ_U8();
		//uint8_t call_retc = READ_U8();
		uint8_t call_argc = arg & 0x0F;
		uint8_t call_retc = arg >> 4;
		printf("Function call!!! argc = %i, retc = %i\n", (int)call_argc, (int)call_retc);
		Value* func_value = state->stack_top - (call_argc + 2);
		if (
			func_value->type != VT_BYTECODE &&
			func_value->type != VT_FOREIGN_FUNCTION
		) {
			if (func_value->type == VT_NONE) {
				printf("PRINTING!!!\n");
				for (size_t i = 0; i < call_argc; i++) {
					print_value(func_value[i + 2]);
					putchar('\n');
				}
				VM_DISPATCH();
			} else
			vm_throw(state, pc,
				"Tried to perform a call on a %s",
				"somethin"
			);
		}
		sno_call(state, argc);
		VM_DISPATCH();
	}
	VM_CASE(OP_RETURN) {
		//READ_U8();
		break;
	}
		
	default:
		// Invalid instruction
		sno_assert(op != 0xFF);
		sno_unreachable;
		break;
	}
	
	return 0;
}
