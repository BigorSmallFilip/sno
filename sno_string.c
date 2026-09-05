#include "sno_string.h"

#include "sno_state.h"

void sno_init_string_interning_table(sno_State* state, uint32_t capacity) {
	sno_assert_ptr(state);
	sno_assert(sno_is_power_of_2(capacity));


}
