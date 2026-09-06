#include "sno_vm.h"



const char* const sno_binop_names[] = {
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"IDIV",
	"MOD",
	"POW",
	"BAND",
	"BOR",
	"BXOR",
	"SHL",
	"SHR",
	"EQ",
	"NEQ",
	"LT",
	"GT",
	"LE",
	"GE",
};

const char* const sno_unop_names[] = {
	"NEG",
	"INC",
	"DEC",
	"BITFLIP",
	"LNOT",
};

const char* const sno_instruction_names[] = {
	"LOAD_NONE",
	"LOAD_FALSE",
	"LOAD_TRUE",
	"LOAD_NUMBER",
	"LOAD_STRING",
	"LOAD_FUNCTION",
	"NEW_ARRAY",
	"NEW_TABLE",
	"DUP",
	"DUP2",
	"POP",
	"GET_LOCAL",
	"SET_LOCAL",
	"GET_GLOBAL",
	"SET_GLOBAL",
	"GET_FIELD",
	"SET_FIELD",
	"GET_INDEX",
	"SET_INDEX",
	"NEW_GLOBAL",
	"GET_METHOD",
	"UNOP",
	"BINOP",
	"TO_BOOL",
	"LAND",
	"LOR",
	"JUMP",
	"JUMP_IF_TRUE",
	"JUMP_IF_FALSE",
	"START_NUMERIC_FORLOOP",
	"END_NUMERIC_FORLOOP",
	"START_CONTAINER_FORLOOP",
	"END_CONTAINER_FORLOOP",
	"CALL",
	"RETURN",
	"HALT",
};

static void print_instruction(const sno_Bytecode* bytecode, const sno_Instruction* i) {
	sno_Instruction instruction = *i;
	uint8_t op = instruction & 0xFF;
	uint8_t arg = instruction >> 8;
	printf("%i > %s ", i - bytecode->instructions, sno_instruction_names[op]);
	switch (op) {
	case sno_I_LOAD_NUMBER:
	{
		printf("%g", bytecode->number_constants[arg]);
		break;
	}
	case sno_I_LOAD_STRING:
	{
		const sno_String* s = bytecode->string_constants[arg];
		printf("\"%.*s\"", s->length, sno_string_chars(s));
		break;
	}
	case sno_I_LOAD_FUNCTION:
	{
		const sno_Bytecode* f = bytecode->sub_functions[arg];
		printf("function: %p", f);
		break;
	}
	case sno_I_GET_LOCAL:
	case sno_I_SET_LOCAL:
	{
		printf("%i", arg);
		break;
	}
	case sno_I_GET_GLOBAL:
	case sno_I_SET_GLOBAL:
	case sno_I_GET_FIELD:
	case sno_I_SET_FIELD:
	case sno_I_NEW_GLOBAL:
	case sno_I_GET_METHOD:
	{
		const sno_String* s = bytecode->string_constants[arg];
		printf("%.*s", s->length, sno_string_chars(s));
		break;
	}
	case sno_I_GET_INDEX:
	case sno_I_SET_INDEX:
	{
		break;
	}
	case sno_I_BINOP:
	{
		printf("%s", sno_binop_names[arg]);
		break;
	}
	case sno_I_UNOP:
	{
		printf("%s", sno_unop_names[arg]);
		break;
	}
	case sno_I_LAND:
	case sno_I_LOR:
	case sno_I_START_NUMERIC_FORLOOP:
	case sno_I_END_NUMERIC_FORLOOP:
	case sno_I_START_CONTAINER_FORLOOP:
	case sno_I_END_CONTAINER_FORLOOP:
	case sno_I_JUMP:
	case sno_I_JUMP_IF_TRUE:
	case sno_I_JUMP_IF_FALSE:
	{
		printf("to %i", i - bytecode->instructions + (int8_t)arg);
		break;
	}
	case sno_I_NEW_ARRAY:
	case sno_I_NEW_TABLE:
	{
		printf("size %i", arg);
		break;
	}
	case sno_I_CALL:
	{
		printf("with %i args", arg);
		break;
	}
	default:
		break;
	}
	printf("\n");
}

void sno_print_bytecode(const sno_Bytecode* bytecode) {
	if (!bytecode) {
		printf("Null bytecode ptr\n");
		return;
	}
	printf("Bytecode: {\n");
	printf("  Instructions size %i: [\n", bytecode->instructions_size);
	for (sno_Instruction* i = bytecode->instructions; i < bytecode->instructions + bytecode->instructions_size; i++) {
		print_instruction(bytecode, i);
	}
	printf("  ]\n");
}
