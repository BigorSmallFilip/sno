#include "sno_gc.h"
#include "sno_state.h"
#include "sno_value.h"

static void mark_all(sno_State* state, uint8_t mark) {
	sno_GCValue* iter = state->gc_start;
	while (iter) {
		iter->gc_mark = mark;
		iter = iter->gc_next;
	}
}

static void mark_globals(sno_State* state) {
	sno_Value* iter;
	size_t bucket;
	sno_table_iter(state->globals, &bucket, &iter);
	while (iter) {
		if (sno_type_is_gc(iter->type)) {
			
		}
		sno_table_next(state->globals, &bucket, &iter);
	}
}

static void mark_stack(sno_State* state) {
	
}

static void mark_live(sno_State* state) {
	
	sno_GCValue* iter;
}

void sno_full_gc(sno_State* state) {
}
