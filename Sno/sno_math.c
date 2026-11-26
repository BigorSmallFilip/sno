#include "sno_math.h"

#include "sno_state.h"

sno_Number lerp(sno_Number a, sno_Number b, sno_Number t) {
	return a + (b - a) * t;
} 

uint32_t random_u32(sno_State* state) {
	sno_assert_ptr(state);
	uint64_t oldseed = state->random_seed;
	state->random_seed = oldseed * 0x5851F42D4C957F2Dull + 0xDA3E39CB94B95BDBull;
	uint32_t xorshifted = (uint32_t)(((oldseed >> 18) ^ oldseed) >> 27);
	uint32_t rot = (uint32_t)(oldseed >> 59);
	return (xorshifted >> rot) | (xorshifted << ((~rot + 1) & 31));
}

sno_Number random_number(sno_State* state) {
	sno_assert_ptr(state);
	uint32_t i = random_u32(state);
	sno_Number n = (sno_Number)i / (sno_Number)UINT32_MAX;
	return n;
}
