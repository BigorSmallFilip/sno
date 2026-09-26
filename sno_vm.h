#ifndef sno_VM_H__
#define sno_VM_H__

#include "sno_utility.h"
#include "sno_value.h"
#include "sno_state.h"

typedef enum {
	sno_BINOP_MUL,
	sno_BINOP_DIV,
	sno_BINOP_IDIV,
	sno_BINOP_ADD,
	sno_BINOP_SUB,
	sno_BINOP_MOD,
	sno_BINOP_POW,
	sno_BINOP_BAND,
	sno_BINOP_BOR,
	sno_BINOP_BXOR,
	sno_BINOP_SHL,
	sno_BINOP_SHR,
	sno_BINOP_LT,
	sno_BINOP_GT,
	sno_BINOP_LE,
	sno_BINOP_GE,
	sno_BINOP_EQ,
	sno_BINOP_NEQ,
	sno_BINOP_LAND,
	sno_BINOP_LOR,
	sno_NOT_BINOP = -1,
} sno_BinOp;

typedef enum {
	sno_UNOP_NEG,
	sno_UNOP_INC,
	sno_UNOP_DEC,
	sno_UNOP_BITFLIP,
	sno_UNOP_LNOT,
	sno_NOT_UNOP = -1,
} sno_UnOp;

enum {
	sno_I_LOAD_NONE,
	sno_I_LOAD_FALSE,
	sno_I_LOAD_TRUE,
	sno_I_LOAD_NUMBER,
	sno_I_LOAD_STRING,
	sno_I_LOAD_FUNCTION,
	sno_I_INTERPOLATE_STRING,
	sno_I_NEW_LINALG,
	sno_I_NEW_ARRAY,
	sno_I_CONCAT_ARRAY,
	sno_I_NEW_TABLE,
	sno_I_CONCAT_TABLE,
	sno_I_COPY,
	sno_I_REV,
	sno_I_POP,
	sno_I_MULTI_ASSIGN_SHUFFLE,
	sno_I_GET_LOCAL,
	sno_I_SET_LOCAL,
	sno_I_GET_GLOBAL,
	sno_I_SET_GLOBAL,
	sno_I_GET_FIELD,
	sno_I_SET_FIELD,
	sno_I_GET_INDEX,
	sno_I_SET_INDEX,
	sno_I_NEW_GLOBAL,
	sno_I_GET_METHOD,
	sno_I_UNOP,
	sno_I_BINOP,
	sno_I_TO_BOOL,
	sno_I_AND,
	sno_I_OR,
	sno_I_JUMP,
	sno_I_JUMP_IF_TRUE,
	sno_I_JUMP_IF_FALSE,
	sno_I_START_NUMERIC_FORLOOP,
	sno_I_END_NUMERIC_FORLOOP,
	sno_I_START_CONTAINER_FORLOOP,
	sno_I_END_CONTAINER_FORLOOP,
	sno_I_CALL,
	sno_I_RETURN,
	sno_I_HALT,
};

typedef struct sno_LocalVar {
	const struct sno_IString* name;
	uint32_t start_pc;
	uint32_t end_pc;
	uint8_t slot;
} sno_LocalVar;

typedef struct sno_Bytecode {
	sno_gc_header;
	const struct sno_IString* name;
	const struct sno_IString* source_code;
	uint16_t num_number_constants;
	uint16_t num_string_constants;
	uint16_t num_sub_functions;
	uint16_t num_local_vars;
	uint8_t local_var_slots;
	uint32_t max_stack_needed;
	uint32_t num_instructions;
	sno_Instruction* instructions;
	sno_Number* number_constants;
	const struct sno_IString** string_constants;
	struct sno_Bytecode** sub_functions;
	sno_LocalVar* local_vars;
	uint32_t* instruction_source_code_offsets;
} sno_Bytecode;

void sno_print_bytecode(const sno_Bytecode* bytecode);
void sno_free_bytecode(sno_State* state, sno_Bytecode* bytecode);

uint8_t sno_execute(sno_State* state, uint8_t num_args);

#endif
