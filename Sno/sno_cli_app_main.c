#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sno.h"

#ifndef sno_BUILD_LIB

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("The Sno Scripting Language\n");
		printf("Specify file to run\n");
		return 0;
	}

	sno_State* state = sno_create_state();

	sno_reserve_stack(state, 2);
	sno_create_string(state, 0, argv[1], strlen(argv[1]));
	sno_copy(state, 0, 1);
	if (!sno_load_string_from_file(state, 1)) {
		printf("exiting program cleanly\n");
		return -1;
	}
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
