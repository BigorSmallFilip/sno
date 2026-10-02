#ifndef sno_MEM_H
#define sno_MEM_H

#include "sno.h"

#include <string.h>



void* state_alloc(sno_GlobalState* state, size_t size);
void* state_realloc(
	sno_GlobalState* state,
	size_t old_size,
	void* block,
	size_t new_size
);
void state_free(sno_GlobalState* state, size_t size, void* block);



#define DECLARE_GENERIC_DYN_ARRAY(type, name, snake_case) \
typedef struct name##DynArray{							  \
	type* buffer;										  \
	size_t count;										  \
	size_t capacity;									  \
} name##DynArray;										  \
														  \
void snake_case##_dyn_array_init(			              \
	name##DynArray* dyn_array							  \
);														  \
void snake_case##_dyn_array_clear(			              \
	sno_VMState* vm,									  \
	name##DynArray* dyn_array							  \
);														  \
void snake_case##_dyn_array_push_n(			              \
	sno_VMState* vm,									  \
	name##DynArray* dyn_array,							  \
	const type* item,									  \
	size_t num_items									  \
);														  \
void snake_case##_dyn_array_push(			              \
	sno_VMState* vm,									  \
	name##DynArray* dyn_array,							  \
	const type* item									  \
)


#define DEFINE_GENERIC_DYN_ARRAY(type, name, snake_case)				 \
void snake_case##_dyn_array_init(										 \
	name##DynArray* dyn_array											 \
) {																		 \
	sno_assert_ptr(dyn_array);											 \
	dyn_array->buffer = NULL;											 \
	dyn_array->count = 0;												 \
	dyn_array->capacity = 0;											 \
}																		 \
																		 \
void snake_case##_dyn_array_clear(										 \
	sno_VMState* vm,													 \
	name##DynArray* dyn_array											 \
) {																		 \
	sno_assert_ptr(vm);													 \
	sno_assert_ptr(dyn_array);											 \
	sno_assert_ptr(dyn_array->buffer);									 \
	state_free(															 \
        vm->state,														 \
		sizeof(type) * dyn_array->capacity,								 \
		dyn_array->buffer												 \
	);										 							 \
	snake_case##_dyn_array_init(dyn_array);								 \
}																		 \
																		 \
void snake_case##_dyn_array_push_n(										 \
	sno_VMState* vm,													 \
	name##DynArray* dyn_array,											 \
	const type* items,													 \
	size_t num_items													 \
) {																		 \
	sno_assert_ptr(vm);													 \
	sno_assert_ptr(items);												 \
	sno_assert_ptr(num_items > 0);										 \
	sno_assert_ptr(dyn_array);											 \
	if (dyn_array->buffer == NULL) {									 \
		sno_assert(dyn_array->count == 0);								 \
		sno_assert(dyn_array->capacity == 0);							 \
		dyn_array->buffer = state_alloc(vm->state, sizeof(type) * 8);	 \
		dyn_array->capacity = 8;										 \
	} else if (dyn_array->count + num_items > dyn_array->capacity) {	 \
		sno_assert(sno_is_power_of_2(dyn_array->capacity));				 \
		size_t new_capacity = dyn_array->capacity << 1;					 \
		dyn_array->buffer = state_realloc(								 \
			vm->state,													 \
			sizeof(type) * dyn_array->capacity,							 \
			dyn_array->buffer,											 \
			sizeof(type) * new_capacity									 \
		);																 \
		dyn_array->capacity = new_capacity;								 \
	}																	 \
	memcpy(																 \
		dyn_array->buffer + dyn_array->count,							 \
		items,															 \
		sizeof(type) * num_items										 \
	);																	 \
}																		 \
																		 \
void snake_case##_dyn_array_push(										 \
	sno_VMState* vm,													 \
	name##DynArray* dyn_array,											 \
	const type* item													 \
) {																		 \
	sno_assert_ptr(vm);													 \
	sno_assert_ptr(item);												 \
	sno_assert_ptr(dyn_array);											 \
	snake_case##_dyn_array_push_n(										 \
		vm,																 \
		dyn_array,														 \
		item,															 \
		1																 \
	);																	 \
}

DECLARE_GENERIC_DYN_ARRAY(uint8_t, Byte, byte);
DECLARE_GENERIC_DYN_ARRAY(sno_Number, Number, number);



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

#endif
