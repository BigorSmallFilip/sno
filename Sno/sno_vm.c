#include "sno_vm.h"

#include "sno_compiler.h"



const OpCodeInfo opcode_info[NUM_OPCODES] = {
	{ 0, 1, "NONE" },
	{ 0, 1, "TRUE" },
	{ 0, 1, "FALSE" },
	{ 0, 2, "NUMBER_IMM8" },
	{ 0, 3, "NUMBER" },
	{ 0, 3, "STRING" },
	{ 0, 3, "BYTECODE" },
	{ 1, 2, "NEW_LINALG" },
	{ 1, 1, "NEW_ARRAY" },
	{ 1, 1, "NEW_TABLE" },
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
	{ 0, 1, "COPY_1" },
	{ 0, 1, "COPY_2" },
	{ 0, 2, "MASH" },
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
	{ 1, 2, "CALL" },
	{ 1, 2, "RETURN" },
	{ 1, 1, "NOP_1" },
	{ 1, 2, "NOP_2" },
	{ 1, 3, "NOP_3" },
};



static void print_instructions(const Bytecode* bytecode) {
	sno_assert_ptr(bytecode);
	PC pc = 0;
	uint32_t pos_i = 0;

	while (pc < bytecode->instructions_size) {
		OpCode opcode = bytecode->instructions[pc];
		sno_assert(opcode < NUM_OPCODES);
		const OpCodeInfo* info = &opcode_info[opcode];
		if (opcode >= OP_NOP_1 && opcode <= OP_NOP_3) {
			pos_i++;
			pc += info->length;
			continue;
		}
		SourceCodePos pos = 0;
		if (info->has_pos) {
			pos = bytecode->instruction_positions[pos_i];
			sno_assert(pos != NO_POS);
			printf("%3u %3u >   %-18s",
				(unsigned int)(pc),
				(unsigned int)pos,
				info->name
			);
			pos_i++;
		} else {
			pos = NO_POS;
			printf("%3u     >   %-18s",
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
		case OP_NUMBER_IMM8: {
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
		case OP_AND:
		case OP_OR: {
			printf("to %u", pc + arg);
		} break;
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
		"Bytecode for function \"%.*s\"\n",
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
