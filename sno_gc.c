#include "sno_gc.h"
#include "sno_state.h"
#include "sno_value.h"

//#define DEBUG_PRINT_GC

static void print_gc_obj(sno_State* state, sno_GCObject* obj) {
	sno_Value v;
	v.type = obj->gc_type + sno_VT_STRING;
	v.v.gc_obj = obj;
	sno_print_value(state, &v);
}



static void mark_all(sno_State* state, uint8_t mark) {
	sno_GCObject* iter = state->gc_list_start;
	size_t objects_marked = 0;
	while (iter) {
#ifdef DEBUG_PRINT_GC
		printf("MARKING ");
		print_gc_obj(state, iter);
		putchar('\n');
#endif
		iter->gc_mark = mark;
		iter = iter->gc_next;
		objects_marked++;
	}
#ifdef DEBUG_PRINT_GC
	printf("Marked %u objects\nNum GC objects = %u\n", objects_marked, state->num_gc_objects);
#endif
}



static void mark_value(sno_State* state, sno_Value* value);



static void mark_array_items(sno_State* state, sno_Array* arr, uint8_t mark) {
	sno_assert(arr->gc_mark == sno_GC_MARK_LIVE);
	for (size_t i = 0; i < arr->items.count; i++) {
		sno_Value* item = &((sno_Value*)arr->items.buffer)[i];
		mark_value(state, item);
	}
}

static void mark_table_items(sno_State* state, sno_Table* table, uint8_t mark) {
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

static void mark_table_and_items(sno_State* state, sno_Table* table, uint8_t mark) {
	table->gc_mark = mark;
	mark_table_items(state, table, mark);
}

static void mark_value(sno_State* state, sno_Value* value) {
	if (!sno_type_is_gc(value->type)) {
		return;
	}
	if (value->v.gc_obj->gc_mark == sno_GC_MARK_LIVE) {
		return;
	}
	value->v.gc_obj->gc_mark = sno_GC_MARK_LIVE;
	if (value->type == sno_VT_ARRAY) {
		mark_array_items(state, value->v.u_array, sno_GC_MARK_LIVE);
	} else if (value->type == sno_VT_TABLE) {
		mark_table_items(state, value->v.u_table, sno_GC_MARK_LIVE);
	} else if (value->type == sno_VT_STRING) {

	} else if (value->type == sno_VT_FUNCTION) {

	}
}



static void mark_stack(sno_State* state) {
	for (size_t i = 0; i < state->stack_top; i++) {
		mark_value(state, &state->stack[i]);
	}
}

static void free_all_objects_marked_grey(sno_State* state, sno_Bool print) {
	sno_GCObject* iter = state->gc_list_start;
	sno_GCObject* prev = NULL;
	while (iter) {
		sno_GCObject* next = iter->gc_next;

		if (print) {
#ifdef DEBUG_PRINT_GC
			printf(iter->gc_mark == sno_GC_MARK_GREY ? "DEAD " : "LIVE ");
			print_gc_obj(state, iter);
			putchar('\n');
#endif
		}

		if (iter->gc_mark == sno_GC_MARK_GREY) {
			sno_free_gc_object(state, iter, prev);
		} else {
			prev = iter;
		}
		iter = next;
	}
}

void sno_full_gc(sno_State* state) {
#ifdef DEBUG_PRINT_GC
	printf(
		"\nFULL GARBAGE COLLECTION PASS\n%ux allocations. %u bytes\n\n",
		(unsigned int)state->num_allocations,
		(unsigned int)state->memory_allocated
	);
#endif

	mark_all(state, sno_GC_MARK_GREY);

	mark_table_and_items(state, state->globals, sno_GC_MARK_LIVE);
	mark_table_and_items(state, state->string_prototype, sno_GC_MARK_LIVE);
	mark_table_and_items(state, state->array_prototype, sno_GC_MARK_LIVE);
	mark_table_and_items(state, state->table_prototype, sno_GC_MARK_LIVE);
	mark_stack(state);

	free_all_objects_marked_grey(state, sno_TRUE);
}
