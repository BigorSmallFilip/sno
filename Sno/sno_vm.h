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
	UNOP_LNOT,
	UNOP_NEG,
	UNOP_BITFLIP,
	UNOP_INC,
	UNOP_DEC,
	NUM_UNOPS,
	NOT_UNOP = -1,
} UnOp;

extern const char* const unop_names[];

enum {
	OP_NONE,
	OP_TRUE,
	OP_FALSE,
	OP_NUMBER_IMM8,
	OP_NUMBER,
	OP_STRING,
	OP_BYTECODE,
	OP_NEW_LINALG,
	OP_NEW_ARRAY,
	OP_NEW_TABLE,
	OP_GET_LOCAL, // 8-bit LocalID
	OP_SET_LOCAL, // 8-bit LocalID
	OP_GET_GLOBAL, // 16-bit string ConstID
	OP_SET_GLOBAL, // 16-bit string ConstID
	OP_GET_FIELD, // 16-bit string ConstID
	OP_SET_FIELD, // 16-bit string ConstID
	OP_GET_INDEX, // imm
	OP_SET_INDEX, // imm
	OP_SET_NEW_GLOBAL, // 16-bit string ConstID
	OP_GET_METHOD, // 16-bit string ConstID

	OP_POP,
	OP_COPY_1,
	OP_COPY_2,
	OP_TO_BOOL,
	OP_TO_BOOL_LNOT,
	OP_NEG,
	OP_BITFLIP,

	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_IDIV,
	OP_MOD,
	OP_POW,
	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_SHL,
	OP_SHR,
	OP_LT,
	OP_GT,
	OP_LE,
	OP_GE,
	OP_EQ,
	OP_NEQ,

	OP_AND, // 16-bit offset forward
	OP_OR, // 16-bit offset forward
	OP_JMP_IF_FALSE, // 16-bit offset forward
	OP_JMP, // 16-bit offset forward
	OP_JMP_BACK, // 16-bit offset backward

	OP_START_NUMERIC_FOR_LOOP, // 16-bit offset forward
	OP_END_NUMERIC_FOR_LOOP, // 16-bit offset backward
	OP_START_CONTAINER_FOR_LOOP, // 16-bit offset forward
	OP_END_CONTAINER_FOR_LOOP, // 16-bit offset backward

	OP_CALL, // 4-bit argc, 4-bit retc
	OP_RETURN, // 8-bit retc
	NUM_OPCODES,
};
typedef uint8_t OpCode;

typedef struct OpCodeInfo {
	uint8_t has_pos;
	uint8_t length; // 1, 2 or 3 bytes
	const char* const name;
} OpCodeInfo;
extern const OpCodeInfo opcode_info[NUM_OPCODES];

typedef uint16_t ConstID;
typedef uint8_t LocalSlot;
typedef uint16_t LocalID;
typedef uint32_t PC; // Program counter

#define MAX_BYTECODE_INSTRUCTIONS (UINT32_MAX - 10)

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
	PC instructions_size;
	PC num_instruction_positions;
	ConstID num_number_constants;
	ConstID num_string_constants;
	ConstID num_bytecode_constants;
	uint8_t* instructions;
	SourceCodePos* instruction_positions;
	sno_Number* number_constants;
	IString** string_constants;
	struct Bytecode** bytecode_constants;
} Bytecode;

void print_bytecode(const Bytecode* bytecode);

#endif
