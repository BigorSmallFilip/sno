#ifndef sno_GC_H
#define sno_GC_H

#include "sno_utility.h"

#define sno_GC_MARK_LIVE 1
#define sno_GC_MARK_DEAD 0

void sno_full_gc(struct sno_State* state);

#endif
