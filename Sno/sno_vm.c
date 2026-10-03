#include "sno_vm.h"

#include "sno_compiler.h"



const char* const opcode_names[NUM_OPCODES] = {
	"NONE",
	"NUMBER",
	"STRING",
	"BYTECODE",
	"GET_LOCAL",
	"SET_LOCAL",
	"GET_GLOBAL",
	"SET_GLOBAL",
	"SET_NEW_GLOBAL",
	"UNOP",
	"BINOP",
	"RETURN",
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
		printf("%5u %5u >   %-16s %5i   ",
			(unsigned int)(pc),
			(unsigned int)bytecode->instruction_positions[pc],
			opcode_names[opcode],
			(int)arg
		);
	} else {
		printf("%5u       >   %-16s %5i   ",
			(unsigned int)(pc),
			opcode_names[opcode],
			(int)arg
		);
	}
	switch (opcode) {
	case OP_NUMBER: {
		sno_assert(arg < bytecode->num_number_constants);
		printf("%g", bytecode->number_constants[arg]);
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
	}
}
