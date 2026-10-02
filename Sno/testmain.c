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

	sno_GlobalState* state = sno_create_state();

	sno_run_test_thing(state);

	sno_free_state(state);

	return 0;
}

#endif
