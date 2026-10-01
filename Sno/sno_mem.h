#ifndef sno_MEM_H
#define sno_MEM_H

#include "sno.h"



enum {
	OT_LINALG,
	OT_STRING,
	OT_ARRAY,
	OT_TABLE,
	OT_FUNCTION,
};
typedef uint8_t GCObjectType;

typedef struct GCObject {
	struct GCObject* gc_next;
	uint8_t gc_mark;
	GCObjectType gc_type;
	uint8_t _padding[6];
} GCObject;
#define gc_object_header \
	struct GCObject* gc_next; uint8_t gc_mark; GCObjectType gc_type
#define gc_object_string_header \
	struct IString* next; uint8_t gc_mark; GCObjectType gc_type



void* state_alloc(sno_GlobalState* state, size_t size);
void state_free(sno_GlobalState* state, size_t size, void* block);

#endif
