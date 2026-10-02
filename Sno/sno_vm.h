#ifndef sno_VM_H
#define sno_VM_H

#include "sno.h"
#include "sno_state.h"

enum {
	OP_NONE,
	OP_NUMBER,
	OP_STRING,
	OP_BYTECODE,
	OP_RETURN,
};
typedef uint8_t OpCode;
typedef uint16_t Instruction;
typedef uint16_t ConstID;

typedef struct Bytecode {
	struct IString* name;
} Bytecode;

#endif
