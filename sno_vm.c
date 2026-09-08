#include "sno_vm.h"

#include "sno_parser.h"
#include <string.h>
#include <math.h>



const char* const sno_binop_names[] = {
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"IDIV",
	"MOD",
	"POW",
	"BAND",
	"BOR",
	"BXOR",
	"SHL",
	"SHR",
	"LT",
	"GT",
	"LE",
	"GE",
	"EQ",
	"NEQ",
};

const char* const sno_binop_fancy_names[] = {
	"add",
	"subtract",
	"multiply",
	"divide",
	"integer divide",
	"mod",
	"pow",
	"bitwise and",
	"bitwise or",
	"bitwise xor",
	"bitwise shift left",
	"bitwise shift right",
	"less than",
	"greater than",
	"less than or equal",
	"greater than or equal",
	"equal",
	"not equal",
};

const char* const sno_unop_names[] = {
	"NEG",
	"INC",
	"DEC",
	"BITFLIP",
	"LNOT",
};

const char* const sno_instruction_names[] = {
	"LOAD_NONE",
	"LOAD_FALSE",
	"LOAD_TRUE",
	"LOAD_NUMBER",
	"LOAD_STRING",
	"LOAD_FUNCTION",
	"NEW_ARRAY",
	"CONCAT_ARRAY",
	"NEW_TABLE",
	"CONCAT_TABLE",
	"COPY",
	"REV",
	"POP",
	"MULTI_ASSIGN_SHUFFLE",
	"GET_LOCAL",
	"SET_LOCAL",
	"GET_GLOBAL",
	"SET_GLOBAL",
	"GET_FIELD",
	"SET_FIELD",
	"GET_INDEX",
	"SET_INDEX",
	"NEW_GLOBAL",
	"GET_METHOD",
	"UNOP",
	"BINOP",
	"TO_BOOL",
	"LAND",
	"LOR",
	"JUMP",
	"JUMP_IF_TRUE",
	"JUMP_IF_FALSE",
	"START_NUMERIC_FORLOOP",
	"END_NUMERIC_FORLOOP",
	"START_CONTAINER_FORLOOP",
	"END_CONTAINER_FORLOOP",
	"CALL",
	"RETURN",
	"HALT",
};

static void print_instruction(const sno_Bytecode* bytecode, const sno_Instruction* i) {
	sno_Instruction instruction = *i;
	uint8_t op = instruction & 0xFF;
	uint8_t arg = instruction >> 8;
	printf("%4u %4u > %s ",
		(unsigned int)(i - bytecode->instructions),
		(unsigned int)bytecode->instruction_source_code_offsets[i - bytecode->instructions],
		sno_instruction_names[op]
	);
	switch (op) {
	case sno_I_LOAD_NUMBER: {
		printf("%g", bytecode->number_constants[arg]);
		break;
	}
	case sno_I_LOAD_STRING: {
		const sno_String* s = bytecode->string_constants[arg];
		printf("\"%.*s\"", (unsigned int)s->length, sno_string_chars(s));
		break;
	}
	case sno_I_LOAD_FUNCTION: {
		const sno_Bytecode* f = bytecode->sub_functions[arg];
		printf("function: %p", f);
		break;
	}
	case sno_I_GET_LOCAL:
	case sno_I_SET_LOCAL: {
		printf("%i", arg);
		break;
	}
	case sno_I_GET_GLOBAL:
	case sno_I_SET_GLOBAL:
	case sno_I_GET_FIELD:
	case sno_I_SET_FIELD:
	case sno_I_NEW_GLOBAL:
	case sno_I_GET_METHOD: {
		const sno_String* s = bytecode->string_constants[arg];
		printf("%.*s", (unsigned int)s->length, sno_string_chars(s));
		break;
	}
	case sno_I_GET_INDEX:
	case sno_I_SET_INDEX: {
		break;
	}
	case sno_I_COPY:
	case sno_I_REV: {
		printf("%ix", arg);
		break;
	}
	case sno_I_POP: {
		break;
	}
	case sno_I_MULTI_ASSIGN_SHUFFLE: {
		printf("stack for %i assignment(s)", arg);
		break;
	}
	case sno_I_BINOP: {
		printf("%s", sno_binop_names[arg]);
		break;
	}
	case sno_I_UNOP: {
		printf("%s", sno_unop_names[arg]);
		break;
	}
	case sno_I_LAND:
	case sno_I_LOR:
	case sno_I_START_NUMERIC_FORLOOP:
	case sno_I_END_NUMERIC_FORLOOP:
	case sno_I_START_CONTAINER_FORLOOP:
	case sno_I_END_CONTAINER_FORLOOP:
	case sno_I_JUMP:
	case sno_I_JUMP_IF_TRUE:
	case sno_I_JUMP_IF_FALSE: {
		printf("to %u", (unsigned int)(i - bytecode->instructions) + (int8_t)arg + 1);
		break;
	}
	case sno_I_NEW_ARRAY:
	case sno_I_CONCAT_ARRAY:
	case sno_I_NEW_TABLE:
	case sno_I_CONCAT_TABLE: {
		printf("size %i", arg);
		break;
	}
	case sno_I_CALL: {
		printf("with %i args, %i returns", arg & 0x0F, arg >> 4);
		break;
	}
	case sno_I_RETURN: {
		printf("%i values", arg);
		break;
	}
	default:
		break;
	}
	printf("\n");
}

void sno_print_bytecode(const sno_Bytecode* bytecode) {
	if (!bytecode) {
		printf("Null bytecode ptr\n");
		return;
	}
	printf("Bytecode: {\n");
	printf("  Instructions size %i: [\n", bytecode->instructions_size);
	for (sno_Instruction* i = bytecode->instructions; i < bytecode->instructions + bytecode->instructions_size; i++) {
		print_instruction(bytecode, i);
	}
	printf("  ]\n");
}



size_t check_array_index(
	sno_State* state,
	sno_Bytecode* bytecode,
	sno_Instruction* pc,
	sno_Array* arr,
	sno_Value* key
) {
	if (key->type != sno_VT_BOOL && key->type != sno_VT_NUMBER) {
		sno_throw_runtime_error_at(
			state,
			bytecode->source_code,
			bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
			"Cannot index into array with %s",
			sno_type_strings_noun[key->type]
		);
	}
	sno_Number index = key->v.u_number;
	if (!sno_number_is_valid_u64(index)) {
		sno_throw_runtime_error_at(
			state,
			bytecode->source_code,
			bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
			"Array index must be an integer. Here it was %g",
			index
		);
	}
	uint64_t i_index = index;
	sno_assert((sno_Number)i_index == index);
	if (i_index >= arr->items.count) {
		sno_throw_runtime_error_at(
			state,
			bytecode->source_code,
			bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
			"Array index was out of bounds. Tried to index item %u but the array only has %i item(s)",
			i_index,
			arr->items.count
		);
	}
	return i_index;
}



uint8_t sno_execute(sno_State* state, uint8_t num_args) {
	sno_Value* base = state->stack + state->stack_base;
	if (base->type != sno_VT_FUNCTION) {
		sno_throw_runtime_error(state, "Called sno_execute on something which wasn't a function");
	}
	sno_Function* function = base->v.u_function;
	const sno_Bytecode* bytecode = function->u.bytecode;
	sno_Instruction* pc = bytecode->instructions;
	sno_Value* locals = base + 1;
	uint8_t multi_assign_offset = 0;
	sno_reserve_stack(state, bytecode->max_stack_needed);
	// This stack pointer is bababa
	sno_Value* stack_ptr = locals + num_args;

	while (1) {
		sno_Instruction i = *pc;
		uint8_t opcode = i & 0xFF;
		uint8_t arg = i >> 8;
		
		//printf("stack %02u   | ", (unsigned int)(stack_ptr - state->stack)); print_instruction(bytecode, pc);
		pc++;

		switch (opcode) {
		case sno_I_LOAD_NONE: {
			stack_ptr++;
			sno_set_none(*stack_ptr);
			break;
		}
		case sno_I_LOAD_FALSE: {
			stack_ptr++;
			sno_set_false(*stack_ptr);
			break;
		}
		case sno_I_LOAD_TRUE: {
			stack_ptr++;
			sno_set_true(*stack_ptr);
			break;
		}
		case sno_I_LOAD_NUMBER: {
			sno_assert(arg < bytecode->num_number_constants);
			stack_ptr++;
			sno_Number number = bytecode->number_constants[arg];
			sno_set_number(*stack_ptr, number);
			break;
		}
		case sno_I_LOAD_STRING: {
			sno_assert(arg < bytecode->num_string_constants);
			stack_ptr++;
			const sno_String* string = bytecode->string_constants[arg];
			sno_set_string(*stack_ptr, string);
			break;
		}
		case sno_I_LOAD_FUNCTION: {
			sno_assert(arg < bytecode->num_sub_functions);
			stack_ptr++;
			sno_Function* function = sno_create_function(state, bytecode->sub_functions[arg]);
			sno_set_function(*stack_ptr, function);
			break;
		}
		case sno_I_NEW_ARRAY: {
			sno_assert(arg <= sno_MAX_STACK_CONSTRUCTOR_ARGS);
			sno_Value* items_start = stack_ptr - arg;
			stack_ptr -= (int)arg - 1;
			sno_Array* arr = sno_create_array(state, 8);
			if (arg > 0) {
				sno_concat_array(state, arr, stack_ptr, arg);
			}
			sno_set_array(*stack_ptr, arr);
			break;
		}
		case sno_I_CONCAT_ARRAY: {
			sno_assert(arg <= sno_MAX_STACK_CONSTRUCTOR_ARGS);
			sno_Value* items_start = stack_ptr - arg;
			stack_ptr -= arg;
			sno_assert(stack_ptr->type == sno_VT_ARRAY);
			sno_Array* arr = stack_ptr->v.u_array;
			if (arg > 0) {
				sno_concat_array(state, arr, stack_ptr + 1, arg);
			}
			break;
		}
		case sno_I_NEW_TABLE: {
			sno_assert(arg <= sno_MAX_STACK_CONSTRUCTOR_ARGS / 2);
			sno_Value* items_start = stack_ptr - arg * 2;
			stack_ptr -= (int)(arg * 2) - 1;
			sno_Table* table = sno_create_table(state, 8);
			for (uint8_t i = 0; i < arg; i++) {
				if (sno_table_set_or_add_key(state, table, &stack_ptr[i * 2], &stack_ptr[i * 2 + 1])) {
					sno_throw_runtime_error_at(
						state,
						bytecode->source_code,
						bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
						"Repetead key"
					);
				}
			}
			sno_set_table(*stack_ptr, table);
			break;
		}
		case sno_I_CONCAT_TABLE: {
			sno_assert(arg <= sno_MAX_STACK_CONSTRUCTOR_ARGS / 2);
			sno_Value* items_start = stack_ptr - arg * 2;
			stack_ptr -= (int)(arg * 2) - 1;
			sno_assert(stack_ptr->type == sno_VT_TABLE);
			sno_Table* table = stack_ptr->v.u_table;
			for (uint8_t i = 0; i < arg; i++) {
				if (sno_table_set_or_add_key(state, table, &stack_ptr[i * 2], &stack_ptr[i * 2 + 1])) {
					sno_throw_runtime_error_at(
						state,
						bytecode->source_code,
						bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
						"Repetead key"
					);
				}
			}
			break;
		}

		case sno_I_COPY: {
			for (uint8_t i = 0; i < arg; i++) {
				stack_ptr++;
				*stack_ptr = stack_ptr[-arg];
			}
			break;
		}
		case sno_I_REV: {
			sno_Value* rev = stack_ptr - arg + 1;
			for (uint8_t i = 0; i < arg >> 1; i++) {
				sno_Value temp = stack_ptr[-i];
				stack_ptr[-i] = rev[i];
				rev[i] = temp;
			}
			break;
		}
		case sno_I_POP: {
			stack_ptr--;
			break;
		}
		case sno_I_MULTI_ASSIGN_SHUFFLE: {
			sno_assert(arg >= 2 && arg <= sno_MAX_STACK_ARGS);
			sno_Value* values = stack_ptr;
			sno_Value* caks = stack_ptr - arg;

			uint8_t num = 0;
			sno_Value shuffled[sno_MAX_STACK_ARGS * 3];
			for (uint8_t i = 0; i < arg; i++) {
				shuffled[sno_MAX_STACK_ARGS * 3 - ++num] = *values;
				values--;
				//printf("GAGAGA %i\n", *(pc + i) >> 8);
				switch ((*(pc + i)) & 0xFF) {
				case sno_I_SET_LOCAL:
				case sno_I_SET_GLOBAL:
					break;
				case sno_I_SET_FIELD:
					shuffled[sno_MAX_STACK_ARGS * 3 - ++num] = *caks; caks--;
					break;
				case sno_I_SET_INDEX:
					shuffled[sno_MAX_STACK_ARGS * 3 - ++num] = *caks; caks--;
					shuffled[sno_MAX_STACK_ARGS * 3 - ++num] = *caks; caks--;
					break;
				default:
					sno_unreachable;
					break;
				}
			}
			sno_Value* bottom = stack_ptr + 1 - num;
			sno_Value* bottom_2 = caks + 1;
			memcpy(bottom, shuffled + sno_MAX_STACK_ARGS * 3 - num, sizeof(sno_Value) * num);
			break;
		}

		case sno_I_GET_LOCAL: {
			sno_assert(arg < bytecode->local_var_slots);
			stack_ptr++;
			*stack_ptr = locals[arg];
			break;
		}
		case sno_I_SET_LOCAL: {
			sno_assert(arg < bytecode->local_var_slots);
			locals[arg] = *stack_ptr;
			stack_ptr--;
			break;
		}
		case sno_I_GET_GLOBAL: {
			sno_assert(arg < bytecode->num_string_constants);
			const sno_String* name = bytecode->string_constants[arg];
			sno_Value key;
			key.type = sno_VT_STRING;
			key.v.u_string = name;
			stack_ptr++;
			if (!sno_table_get(state->globals, &key, stack_ptr)) {
				sno_throw_runtime_error_at(
					state,
					bytecode->source_code,
					bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
					"Couldn't find any variable named '%.*s'",
					name->length,
					sno_string_chars(name)
				);
			}
			break;
		}
		case sno_I_SET_GLOBAL: {
			sno_assert(arg < bytecode->num_string_constants);
			const sno_String* name = bytecode->string_constants[arg];
			sno_Value key;
			key.type = sno_VT_STRING;
			key.v.u_string = name;
			if (!sno_table_set(state->globals, &key, stack_ptr)) {
				sno_throw_runtime_error_at(
					state,
					bytecode->source_code,
					bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
					"Couldn't find any variable named '%.*s'",
					name->length,
					sno_string_chars(name)
				);
			}
			stack_ptr--;
			break;
		}
		case sno_I_GET_FIELD: {
			sno_assert(arg < bytecode->num_string_constants);
			const sno_String* key_name = bytecode->string_constants[arg];
			sno_Value key;
			key.type = sno_VT_STRING;
			key.v.u_string = key_name;
			switch (stack_ptr->type) {
			case sno_VT_TABLE: {
				if (!sno_table_get(stack_ptr->v.u_table, &key, stack_ptr)) {
					sno_throw_runtime_error_at(
						state,
						bytecode->source_code,
						bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
						"Table has no field named %.*s",
						key_name->length,
						sno_string_chars(key_name)
					);
				}
				break;
			}
			default:
				sno_not_implemented;
				break;
			}
			break;
		}
		case sno_I_SET_FIELD: {
			sno_assert(arg < bytecode->num_string_constants);
			sno_Value* value = stack_ptr;
			sno_Value* container = stack_ptr - 1;
			stack_ptr -= 2;
			const sno_String* key_name = bytecode->string_constants[arg];
			sno_Value key;
			key.type = sno_VT_STRING;
			key.v.u_string = key_name;
			switch (container->type) {
			case sno_VT_TABLE: {
				if (!sno_table_set(container->v.u_table, &key, value)) {
					sno_throw_runtime_error_at(
						state,
						bytecode->source_code,
						bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
						"Table has no field named %.*s",
						key_name->length,
						sno_string_chars(key_name)
					);
				}
				break;
			}
			default:
				sno_not_implemented;
				break;
			}
			break;
		}
		case sno_I_GET_INDEX: {
			sno_Value* container = stack_ptr - 1;
			sno_Value* key = stack_ptr;
			stack_ptr--;
			sno_Value* result = stack_ptr;
			switch (container->type) {
			case sno_VT_ARRAY: {
				sno_Array* arr = container->v.u_array;
				size_t index = check_array_index(state, bytecode, pc, arr, key);
				*result = ((sno_Value*)arr->items.buffer)[index];
				break;
			}
			case sno_VT_TABLE: {
				sno_Table* table = container->v.u_table;
				sno_table_get(table, key, stack_ptr);
				break;
			}
			default:
				sno_throw_runtime_error_at(
					state,
					bytecode->source_code,
					bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
					"Cannot index into %s",
					sno_type_strings_noun[container->type]
				);
			}
			break;
		}
		case sno_I_SET_INDEX: {
			sno_Value* value = stack_ptr;
			sno_Value* key = stack_ptr - 1;
			sno_Value* container = stack_ptr - 2;
			stack_ptr -= 3;
			switch (container->type) {
			case sno_VT_ARRAY: {
				sno_Array* arr = container->v.u_array;
				size_t index = check_array_index(state, bytecode, pc, arr, key);
				((sno_Value*)arr->items.buffer)[index] = *value;
				break;
			}
			case sno_VT_TABLE: {
				sno_Table* table = container->v.u_table;
				sno_table_set_or_add_key(state, table, key, value);
				break;
			}
			default:
				sno_throw_runtime_error_at(
					state,
					bytecode->source_code,
					bytecode->instruction_source_code_offsets[pc - bytecode->instructions],
					"Cannot index into %s",
					sno_type_strings_noun[container->type]
				);
			}
			break;
		}

		case sno_I_NEW_GLOBAL: {
			sno_assert(arg < bytecode->num_string_constants);
			const sno_String* name = bytecode->string_constants[arg];
			sno_Value key;
			key.type = sno_VT_STRING;
			key.v.u_string = name;
			if (sno_table_set_or_add_key(state, state->globals, &key, stack_ptr)) {
				sno_throw_runtime_error_at(
					state,
					bytecode->source_code,
					bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
					"A global variable named '%.*s' already exists",
					name->length,
					sno_string_chars(name)
				);
			}
			stack_ptr--;
			break;
		}

		case sno_I_BINOP: {
			stack_ptr--;
			if (arg >= sno_BINOP_ADD && arg <= sno_BINOP_GE) {
				sno_ValueType type_l = stack_ptr[0].type;
				sno_ValueType type_r = stack_ptr[1].type;
				if (!(type_l == sno_VT_BOOL || type_l == sno_VT_NUMBER) ||
					!(type_r == sno_VT_BOOL || type_r == sno_VT_NUMBER)) {
					sno_throw_runtime_error_at(
						state,
						bytecode->source_code,
						bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
						"Attempted to %s %s and %s",
						sno_binop_fancy_names[arg],
						sno_type_strings_noun[type_l],
						sno_type_strings_noun[type_r]
					);
				}
				sno_ValueType* result_type = &stack_ptr[0].type;
				*result_type = sno_VT_NUMBER;
				sno_Number rhs = stack_ptr[1].v.u_number;
				sno_Number* result = &stack_ptr[0].v.u_number;
				switch (arg) {
				case sno_BINOP_ADD: *result += rhs; break;
				case sno_BINOP_SUB: *result -= rhs; break;
				case sno_BINOP_MUL: *result *= rhs; break;
				case sno_BINOP_DIV: *result /= rhs; break;
				case sno_BINOP_IDIV: *result = sno_idiv(*result, rhs); break;
				case sno_BINOP_MOD: *result = sno_mod(*result, rhs); break;
				case sno_BINOP_POW: *result = sno_pow(*result, rhs); break;
				case sno_BINOP_BAND: *result = (sno_Number)(((sno_Int)*result) & ((sno_Int)rhs)); break;
				case sno_BINOP_BOR: *result = (sno_Number)(((sno_Int)*result) | ((sno_Int)rhs)); break;
				case sno_BINOP_BXOR: *result = (sno_Number)(((sno_Int)*result) ^ ((sno_Int)rhs)); break;
				case sno_BINOP_SHL: *result = (sno_Number)(((sno_Int)*result) << ((sno_Int)rhs)); break;
				case sno_BINOP_SHR: *result = (sno_Number)(((sno_Int)*result) >> ((sno_Int)rhs)); break;
				case sno_BINOP_LT: *result = (sno_Number)(*result < rhs); *result_type = sno_VT_BOOL; break;
				case sno_BINOP_GT: *result = (sno_Number)(*result > rhs); *result_type = sno_VT_BOOL; break;
				case sno_BINOP_LE: *result = (sno_Number)(*result <= rhs); *result_type = sno_VT_BOOL; break;
				case sno_BINOP_GE: *result = (sno_Number)(*result >= rhs); *result_type = sno_VT_BOOL; break;
				}
			}
			break;
		}

		case sno_I_JUMP: {
			pc += (int8_t)arg;
			break;
		}
		case sno_I_JUMP_IF_FALSE: {
			if (!sno_value_to_bool(stack_ptr)) {
				pc += (int8_t)arg;
			}
			stack_ptr--;
			break;
		}
		case sno_I_START_NUMERIC_FORLOOP: {
			sno_Value* start = stack_ptr - 2;
			sno_Value* stop = stack_ptr - 1;
			sno_Value* step = stack_ptr - 0;
			if (start->type != sno_VT_NUMBER ||
				stop->type != sno_VT_NUMBER ||
				step->type != sno_VT_NUMBER
			) {
				sno_throw_runtime_error_at(
					state,
					bytecode->source_code,
					bytecode->instruction_source_code_offsets[pc - 1 - bytecode->instructions],
					"For loop was given non number arguments"
				);
			}
			if (start->v.u_number < stop->v.u_number) {
				// Push the iter number for the STORE_LOCAL instruction after this
				stack_ptr++;
				sno_set_number(*stack_ptr, start->v.u_number);
			} else {
				stack_ptr -= 3; // Discard the loop variables
				pc += (int8_t)arg; // Jump out
			}
			break;
		}
		case sno_I_END_NUMERIC_FORLOOP: {
			sno_Value* start = stack_ptr - 2;
			sno_Value* stop = stack_ptr - 1;
			sno_Value* step = stack_ptr - 0;
			sno_assert(start->type == sno_VT_NUMBER);
			sno_assert(stop->type == sno_VT_NUMBER);
			sno_assert(step->type == sno_VT_NUMBER);
			start->v.u_number += step->v.u_number;
			pc += (int8_t)arg;
			break;
		}

		case sno_I_CALL: {
			uint8_t num_args = arg & 0x0F;
			uint8_t num_returns = arg >> 4;
			uint32_t saved_base = state->stack_base;
			uint32_t stack_idx = stack_ptr - state->stack;
			state->stack_base = stack_idx - (num_args + 1);
			sno_call(state, num_args, num_returns);
			state->stack_base = saved_base;
			stack_ptr = state->stack + stack_idx - 2 - num_args + num_returns;
			break;
		}
		case sno_I_RETURN: {
			sno_assert(arg < sno_MAX_STACK_ARGS);
			for (uint8_t i = 0; i < arg; i++) {
				base[i] = stack_ptr[-arg + 1 + i];
			}
			return arg;
		}
		default: {
			sno_throw_runtime_error(state, "Invalid instruction executed");
		}
		}
	}
}
