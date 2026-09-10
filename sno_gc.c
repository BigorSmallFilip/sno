#include "sno_gc.h"
#include "sno_state.h"
#include "sno_value.h"

static void mark_all(sno_State* state, uint8_t mark) {
	sno_GCValue* iter = state->gc_list_start;
	while (iter) {
		iter->gc_mark = mark;
		iter = iter->gc_next;
	}
}



static void mark_value(sno_State* state, sno_Value* value);



static void mark_array_items(sno_State* state, sno_Array* arr) {
	for (size_t i = 0; i < arr->items.count; i++) {
		sno_Value* item = &((sno_Value*)arr->items.buffer)[i];
		mark_value(state, item);
	}
}

static void mark_value(sno_State* state, sno_Value* value) {
	if (value->type == sno_VT_ARRAY) {
		value->v.u_array->gc_mark = sno_GC_MARK_LIVE;
		mark_array_items(state, value->v.u_array);
	} else if (value->type == sno_VT_TABLE) {

	} else if (value->type == sno_VT_STRING) {

	}
}

static void mark_table_items(sno_State* state, sno_Table* table) {
	sno_TableNode* iter;
	size_t bucket;
	sno_table_iter(table, &bucket, &iter);
	while (iter) {
		sno_Value key;
		key.type = iter->key_type;
		key.v.u_data = iter->key_union.u_data;
		sno_Value value;
		value.type = iter->value_type;
		value.v.u_data = iter->value_union.u_data;
		mark_value(state, &key);
		mark_value(state, &value);
		sno_table_next(table, &bucket, &iter);
	}
}

static void mark_stack(sno_State* state) {
	
}

static void mark_live(sno_State* state) {
	
	sno_GCValue* iter;
}

void sno_full_gc(sno_State* state) {
	mark_all(state, sno_GC_MARK_GREY);
	mark_table_items(state, state->globals);
}
