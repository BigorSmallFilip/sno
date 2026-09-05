#include <stdio.h>

#include "sno_utility.h"
#include "sno_state.h"
#include "sno_parser.h"
#include "sno_string.h"

EMSCRIPTEN_EXPORT int main(int argc, char** argv) {
	printf("Hello Sno\n");

	if (argc == 2) {
		printf("With an arg \"%s\"\n", argv[1]);
	}

	sno_State* state = sno_create_state();

	const sno_String* name = sno_create_string_from_literal(state, "What");

	sno_print_string_interning_table(state);

	sno_print_source_code(state, sno_str_comma_len("what"));
	sno_free_state(state);

	return 0;
}
