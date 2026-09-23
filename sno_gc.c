#include "sno_gc.h"
#include "sno_state.h"
#include "sno_value.h"
#include "sno_vm.h"

//#define DEBUG_PRINT_GC_EVERYTHING
#define DEBUG_PRINT_GC_RESULT

static void print_gc_obj(sno_State* state, sno_GCObject* obj) {
	if (obj->gc_type >= sno_OT_BYTECODE) {
		sno_Bytecode* bytecode = (sno_Bytecode*)obj;
		printf(
			"Bytecode 0x%p \"%.*s\"",
			bytecode,
			bytecode->name->length,
			sno_string_chars(bytecode->name)
		);
		return;
	}
	sno_Value v;
	v.type = obj->gc_type + sno_VT_STRING;
	v.v.gc_obj = obj;
	sno_print_value(state, &v);
}



static void mark_value(sno_State* state, sno_Value* value);

static void mark_string(sno_State* state, const sno_IString* string) {
	sno_IString* mut_string = (sno_IString*)string;
	if (mut_string->gc_mark == sno_GC_MARK_LIVE) {
		return;
	}
	mut_string->gc_mark = sno_GC_MARK_LIVE;
	/*printf(
		sno_ANSI_BLUE "safe: \"%.*s\"\n" sno_ANSI_NORMAL,
		(unsigned int)mut_string->length,
		sno_string_chars(mut_string)
	);*/
}

static void mark_array_items(sno_State* state, sno_Array* arr) {
	if (!sno_type_is_gc(arr->items_type)) {
		return;
	}
	for (size_t i = 0; i < arr->items.count; i++) {
		sno_Value* item = &((sno_Value*)arr->items.buffer)[i];
		mark_value(state, item);
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

static void mark_bytecode(sno_State* state, sno_Bytecode* bytecode) {
	if (bytecode->gc_mark == sno_GC_MARK_LIVE) {
		return;
	}
	bytecode->gc_mark = sno_GC_MARK_LIVE;
	for (uint32_t i = 0; i < (uint32_t)bytecode->num_string_constants; i++) {
		mark_string(state, bytecode->string_constants[i]);
	}
	for (uint32_t i = 0; i < (uint32_t)bytecode->num_local_vars; i++) {
		mark_string(state, bytecode->local_vars[i].name);
	}
	mark_string(state, bytecode->name);
	mark_string(state, bytecode->source_code);
	for (uint32_t i = 0; i < (uint32_t)bytecode->num_sub_functions; i++) {
		sno_Bytecode* sub_function = bytecode->sub_functions[i];
		mark_bytecode(state, sub_function);
	}
}

static void mark_function(sno_State* state, sno_Function* function) {
	if (function->is_c_function) {
		return;
	}
	mark_bytecode(state, function->u.bytecode);
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
		mark_array_items(state, value->v.u_array);
	} else if (value->type == sno_VT_TABLE) {
		mark_table_items(state, value->v.u_table);
	} else if (value->type == sno_VT_STRING) {
		mark_string(state, value->v.u_string);
	} else if (value->type == sno_VT_FUNCTION) {
		mark_function(state, value->v.u_function);
	}
}

static void mark_stack(sno_State* state) {
	for (size_t i = 0; i < state->stack_top; i++) {
		mark_value(state, &state->stack[i]);
	}
}



static void sweep(sno_State* state) {
	sno_GCObject* iter = state->gc_list_start;
	sno_GCObject* prev = NULL;
	while (iter) {
		sno_GCObject* next = iter->gc_next;
#ifdef DEBUG_PRINT_GC_EVERYTHING
		printf(
			iter->gc_mark == sno_GC_MARK_DEAD ?
			sno_ANSI_RED "DEAD: " :
			sno_ANSI_GREEN "LIVE: "
		);
		print_gc_obj(state, iter);
		putchar('\n');
#endif
		if (iter->gc_mark == sno_GC_MARK_DEAD) {
			sno_free_gc_object(state, iter, prev);
		} else {
			iter->gc_mark = sno_GC_MARK_DEAD;
			prev = iter;
		}
		iter = next;
	}
}

static void sweep_strings(sno_State* state) {
	sno_StringInterningTable* str_table = &state->string_table;
	for (size_t i = 0; i < str_table->capacity_mask + 1; i++) {
		sno_IString* iter = str_table->strings[i];
		sno_IString* prev = NULL;
		while (iter) {
#ifdef DEBUG_PRINT_GC_EVERYTHING
			printf(
				iter->gc_mark == sno_GC_MARK_DEAD ?
				sno_ANSI_RED "DEAD: " :
				sno_ANSI_GREEN "LIVE: "
			);
			putchar('"');
			print_gc_obj(state, iter);
			putchar('"');
			putchar('\n');
#endif
			sno_IString* next = iter->next;
			if (iter->gc_mark == sno_GC_MARK_DEAD) {
				sno_free_string(state, iter, prev);
				if (str_table->strings[i] == iter) {
					str_table->strings[i] = next;
				}
			} else {
				iter->gc_mark = sno_GC_MARK_DEAD;
				prev = iter;
			}
			iter = next;
		}
	}
}



static void print_all_gc_objects(sno_State* state) {
	size_t num_iters = 0;
	sno_GCObject* iter = state->gc_list_start;
	printf(sno_ANSI_CYAN "Here are all GC objects in the list\n");
	while (iter) {
		printf("%llu = ", num_iters);
		print_gc_obj(state, iter);
		putchar('\n');
		//iter->gc_mark = sno_GC_MARK_LIVE;
		iter = iter->gc_next;
		num_iters++;
	}
	printf(
		"Found %llu when iterating the %llu objects\n",
		num_iters,
		state->num_non_string_gc_objects
	);
}



static void mark_roots(sno_State* state) {
	state->globals->gc_mark = sno_GC_MARK_LIVE;
	mark_table_items(state, state->globals);
	state->string_prototype->gc_mark = sno_GC_MARK_LIVE;
	mark_table_items(state, state->string_prototype);
	state->array_prototype->gc_mark = sno_GC_MARK_LIVE;
	mark_table_items(state, state->array_prototype);
	state->table_prototype->gc_mark = sno_GC_MARK_LIVE;
	mark_table_items(state, state->table_prototype);
	mark_stack(state);
	if (state->exception_msg) {
		sno_IString* exception_msg = (sno_IString*)state->exception_msg;
		exception_msg->gc_mark = sno_GC_MARK_LIVE;
	}
}

void sno_full_gc(sno_State* state) {
#ifdef DEBUG_PRINT_GC_EVERYTHING
	printf(
		"\nFULL EXHAUSTIVE GARBAGE COLLECTION PASS\n%ux allocations. %u bytes\n\n",
		(unsigned int)state->num_allocations,
		(unsigned int)state->memory_allocated
	);
#endif
	double start_time = sno_perftimer();
	
#ifdef DEBUG_PRINT_GC_EVERYTHING
	print_all_gc_objects(state);
#endif

	mark_roots(state);

	size_t memory_before = state->memory_allocated;
	size_t num_allocations_before = state->num_allocations;

#ifdef DEBUG_PRINT_GC_EVERYTHING
	printf(sno_ANSI_PURPLE " === GC OBJECTS ===\n" sno_ANSI_NORMAL);
#endif
	sweep(state);
	sweep_strings(state);
#ifdef DEBUG_PRINT_GC_EVERYTHING
	printf("\n");
#endif

	size_t memory_freed = memory_before - state->memory_allocated;
	size_t num_allocations_freed = num_allocations_before - state->num_allocations;

	state->live_memory_last_gc = state->memory_allocated;

	double duration = sno_perftimer() - start_time;
#ifdef DEBUG_PRINT_GC_RESULT
	printf(
		sno_ANSI_PURPLE
		"GARBAGE COLLECTION PASS COMPLETE AFTER %gms\n"
		"%u allocations, %u bytes freed\n"
		"Now live:\n"
		"%u allocations, %u bytes\n\n"
		sno_ANSI_NORMAL,
		duration * 1000.0,
		(unsigned int)num_allocations_freed,
		(unsigned int)memory_freed,
		(unsigned int)state->num_allocations,
		(unsigned int)state->memory_allocated
	);
#endif
}



void sno_consider_gc(sno_State* state) {
	sno_assert_ptr(state);
	if (
		(state->memory_allocated > state->live_memory_last_gc << 1) ||
		(
			(state->memory_allocated > sno_MIN_MEMORY_BEFORE_GC) &&
			state->memory_allocated < state->live_memory_last_gc >> 2
		)
	) {
		sno_full_gc(state);
	}
}
