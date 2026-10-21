#include <stdio.h>

#include "sno.h"

#ifndef sno_BUILD_LIB

int main(int argc, char** argv) {
	sno_assert(argc <= 2);
	if (argc == 1) {
		printf("The Sno Scripting Language\n");
		return 0;
	} else if (argc == 2) {
		printf("Arg = \"%s\"\n", argv[1]);
	}

	sno_State* state = sno_create_state();

	sno_reserve_stack(state, 2);
	sno_create_string(state, 0, sno_string_comma_length("test.sno"));
	sno_copy(state, 0, 1);
	sno_load_string_from_file(state, 1);
	if (!sno_compile(state)) {
		sno_print_error_message(state);
		return -1;
	}
	sno_reserve_stack(state, 1);
	sno_set_none(state, 1);
	sno_call(state, 0, 0);

	//sno_run_test_thing(state);

	sno_free_state(state);

	return 0;
}

#endif
