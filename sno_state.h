#ifndef sno_STATE_H
#define sno_STATE_H

#include "sno_utility.h"

typedef struct sno_State {
	int temp;
} sno_State;

sno_API sno_State* sno_create_state();
sno_API void sno_free_state(sno_State* state);

#endif
