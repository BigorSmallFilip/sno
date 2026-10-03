#ifndef sno_VM_H
#define sno_VM_H

#include "sno.h"
#include "sno_state.h"

typedef enum {
	BINOP_ADD,
	BINOP_SUB,
	BINOP_MUL,
	BINOP_DIV,
	BINOP_IDIV,
	BINOP_MOD,
	BINOP_POW,
	BINOP_BAND,
	BINOP_BOR,
	BINOP_BXOR,
	BINOP_SHL,
	BINOP_SHR,
	BINOP_LT,
	BINOP_GT,
	BINOP_LE,
	BINOP_GE,
	BINOP_EQ,
	BINOP_NEQ,
	BINOP_LAND,
	BINOP_LOR,
	NUM_BINOPS,
	NOT_BINOP = -1,
} BinOp;

extern const char* const binop_names[];

typedef enum {
	UNOP_NEG,
	UNOP_INC,
	UNOP_DEC,
	UNOP_BITFLIP,
	UNOP_LNOT,
	NUM_UNOPS,
	NOT_UNOP = -1,
} UnOp;

extern const char* const unop_names[];

enum {
	OP_NONE,
	OP_BOOL,
	OP_NUMBER,
	OP_STRING,
	OP_BYTECODE,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_SET_NEW_GLOBAL,
	OP_UNOP,
	OP_BINOP,
	OP_RETURN,
	NUM_OPCODES,
};
typedef uint8_t OpCode;

extern const char* const opcode_names[];

typedef uint16_t Instruction;
typedef uint16_t ConstID;
typedef uint8_t LocalSlot;
typedef uint16_t LocalID;
typedef uint32_t PC; // Program counter

#define MAX_BYTECODE_INSTRUCTIONS (UINT32_MAX - 1)

typedef struct LocalVar {
	struct IString* name;
	LocalSlot slot;
	PC start_pc;
	PC end_pc;
} LocalVar;

typedef struct Bytecode {
	struct IString* name;
	struct IString* source_code_name;
	struct IString* source_code;
	PC num_instructions;
	ConstID num_number_constants;
	ConstID num_string_constants;
	ConstID num_bytecode_constants;
	Instruction* instructions;
	SourceCodePos* instruction_positions;
	sno_Number* number_constants;
	IString** string_constants;
	struct Bytecode** bytecode_constants;
} Bytecode;

void print_bytecode(const Bytecode* bytecode);

#endif
