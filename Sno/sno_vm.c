#include "sno_vm.h"

#include "sno_compiler.h"



const char* const binop_names[NUM_BINOPS] = {
	"+",
	"-",
	"*",
	"/",
	"/-",
	"%",
	"**",
	"&",
	"|",
	"^",
	"<<",
	">>",
	"<",
	">",
	"<=",
	">=",
	"==",
	"!=",
	"and",
	"or",
};

const char* const unop_names[NUM_UNOPS] = {
	"-",
	"++",
	"--",
	"~",
	"not",
};

const char* const opcode_names[NUM_OPCODES] = {
	"NONE",
	"BOOL",
	"NUMBER",
	"STRING",
	"BYTECODE",
	"NEW_LINALG",
	"NEW_ARRAY",
	"NEW_TABLE",
	"GET_LOCAL",
	"SET_LOCAL",
	"GET_GLOBAL",
	"SET_GLOBAL",
	"GET_FIELD",
	"SET_FIELD",
	"GET_INDEX",
	"SET_INDEX",
	"SET_NEW_GLOBAL",
	"GET_METHOD",
	"COPY",
	"MASH", // Multi-assign shuffle
	"TO_BOOL",
	"UNOP",
	"BINOP",
	"AND",
	"OR",
	"JUMP_FRWD_IF_FALSE",
	"JUMP_FRWD",
	"JUMP_BACK",
	"CALL",
	"RETURN",
	"NOP",
};

static sno_Bool print_instruction(const Bytecode* bytecode, PC pc) {
	sno_assert_ptr(bytecode);
	sno_assert(pc < bytecode->num_instructions);
	Instruction instruction = bytecode->instructions[pc];
	OpCode opcode = instruction & 0xFF;
	sno_assert(opcode < NUM_OPCODES);
	uint16_t arg = instruction >> 8;
	sno_Bool is_extended = sno_FALSE;
	if (arg == 0xFF) {
		arg = (uint16_t)bytecode->instructions[pc + 1];
		is_extended = sno_TRUE;
	}
	if (bytecode->instruction_positions[pc] != NO_POS) {
		printf("%3u %3u >   %-18s %5i   ",
			(unsigned int)(pc),
			(unsigned int)bytecode->instruction_positions[pc],
			opcode_names[opcode],
			(int)arg
		);
	} else {
		printf("%3u     >   %-18s %5i   ",
			(unsigned int)(pc),
			opcode_names[opcode],
			(int)arg
		);
	}
	switch (opcode) {
	case OP_BOOL: {
		sno_assert(arg == 0 || arg == 1);
		printf("%s", arg ? "true" : "false");
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
	case OP_BINOP: {
		sno_assert(arg < NUM_BINOPS);
		printf("%s", binop_names[arg]);
	} break;
	case OP_UNOP: {
		sno_assert(arg < NUM_UNOPS);
		printf("%s", unop_names[arg]);
	} break;
	case OP_JUMP_FRWD_IF_FALSE:
	case OP_JUMP_FRWD:
	case OP_AND:
	case OP_OR: {
		printf("to %u", pc + arg);
	} break;
	case OP_JUMP_BACK: {
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
	return is_extended;
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
	for (PC i = 0; i < bytecode->num_instructions; i++) {
		i += print_instruction(bytecode, i);
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
	}
}
