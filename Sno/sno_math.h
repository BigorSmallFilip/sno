#ifndef sno_MATH_H
#define sno_MATH_H

#include "sno.h"
#include <math.h>

sno_Number lerp(sno_Number a, sno_Number b, sno_Number t);
uint32_t random_u32(sno_State* state);
sno_Number random_number(sno_State* state);

#endif
