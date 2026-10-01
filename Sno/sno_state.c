#include "sno_state.h"

#include <stdlib.h>

sno_API sno_State* sno_create_state(void) {
	sno_State* state = malloc(sizeof(sno_State));
	if (!state) {
		return NULL;
	}
	state->temp = 0;
	return state;
}

sno_API void sno_free_state(sno_State* state) {
	sno_assert_ptr(state);
	free(state);
}
