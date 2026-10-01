#include <stdio.h>
#include <sno.h>

int main() {
	printf("Hello\n");

	sno_GlobalState* state = sno_create_state();
	sno_run_test_thing(state);
	sno_free_state(state);

	return 0;
}
