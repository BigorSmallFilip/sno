#include "sno_vm.h"

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
}
