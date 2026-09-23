#include "sno_lib.h"
#include "sno_state.h"

#include <math.h>

uint8_t snol_string_length(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	const sno_IString* string = sno_get_arg(state, sno_self)->v.u_string;
	sno_set_ret_number(state, 0, string->length);
	return 1;
}

uint8_t snol_string_to_lowercase(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	const sno_IString* string = sno_get_arg(state, sno_self)->v.u_string;
	sno_set_ret_string(state, 0, sno_string_to_lowercase(state, string));
	return 1;
}

uint8_t snol_string_to_uppercase(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	const sno_IString* string = sno_get_arg(state, sno_self)->v.u_string;
	sno_set_ret_string(state, 0, sno_string_to_uppercase(state, string));
	return 1;
}

uint8_t snol_string_is_lowercase(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	const sno_IString* string = sno_get_arg(state, sno_self)->v.u_string;
	const char* chars = sno_string_chars(string);
	sno_Bool result = sno_TRUE;
	for (size_t i = 0; i < string->length; i++) {
		if (chars[i] >= 'A' && chars <= 'Z') {
			result = sno_FALSE;
			break;
		}
	}
	sno_set_ret_bool(state, 0, result);
	return 1;
}

uint8_t snol_string_is_uppercase(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 0);
	const sno_IString* string = sno_get_arg(state, sno_self)->v.u_string;
	const char* chars = sno_string_chars(string);
	sno_Bool result = sno_TRUE;
	for (size_t i = 0; i < string->length; i++) {
		if (chars[i] >= 'a' && chars <= 'z') {
			result = sno_FALSE;
			break;
		}
	}
	sno_set_ret_bool(state, 0, result);
	return 1;
}

uint8_t snol_string_slice(sno_State* state, uint8_t num_args) {
	sno_check_arg_count(state, num_args, 2);
	const sno_IString* string = sno_get_arg_typed(state, sno_VT_STRING, sno_self)->v.u_string;
	sno_Number start = sno_get_arg_typed(state, sno_VT_NUMBER, 0)->v.u_number;
	sno_Number end = sno_get_arg_typed(state, sno_VT_NUMBER, 1)->v.u_number;
	if (!sno_number_is_valid_u32(start) ||
		!sno_number_is_valid_u32(end)) {
		sno_throw_runtime_error(state, "String index wasn't integer");
	}
	uint32_t istart = (uint32_t)start;
	uint32_t iend   = (uint32_t)end;
	if (istart > iend) {
		sno_throw_runtime_error(state, "Slice index starts after it ends");
	}
	if ((istart > string->length) || (iend > string->length)) {
		sno_throw_runtime_error(state, "Slice index out of bounds");
	}
	uint32_t length = iend - istart;
	sno_set_ret_string(
		state,
		0,
		sno_create_string(state, &sno_string_chars(string)[istart], length)
	);
	return 1;
}

const sno_Library sno_lib_string[] = {
	sno_LibFn(string, length),
	sno_LibFn(string, to_lowercase),
	sno_LibFn(string, to_uppercase),
	sno_LibFn(string, is_lowercase),
	sno_LibFn(string, is_uppercase),
	sno_LibFn(string, slice),
	sno_LibEnd
};
