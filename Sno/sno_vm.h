#ifndef sno_VM_H
#define sno_VM_H

#include "sno.h"
#include "sno_state.h"

enum {
	OP_NONE,
	OP_NUMBER,
	OP_STRING,
	OP_BYTECODE,
	OP_GET_LOCAL,
	OP_SET_LOCAL,
	OP_GET_GLOBAL,
	OP_SET_GLOBAL,
	OP_RETURN,
};
typedef uint8_t OpCode;
typedef uint16_t Instruction;
typedef uint16_t ConstID;
typedef uint8_t LocalSlot;
typedef uint16_t LocalID;
typedef uint32_t PC; // Program counter

#define MAX_BYTECODE_INSTRUCTIONS UINT32_MAX

typedef struct LocalVar {
	struct IString* name;
	LocalSlot slot;
	PC start_pc;
	PC end_pc;
} LocalVar;

typedef struct Bytecode {
	struct IString* name;
} Bytecode;

#endif
