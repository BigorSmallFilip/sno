#include <stdio.h>

#include "sno_utility.h"
#include "sno_state.h"

EMSCRIPTEN_EXPORT int main(int argc, char** argv) {
	printf("Hello Sno\n");

	if (argc == 2) {
		printf("With an arg \"%s\"\n", argv[1]);
	}

	sno_State* state = sno_create_state();

	sno_free_state(state);

	return 0;
}
