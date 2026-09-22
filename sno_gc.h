#ifndef sno_GC_H
#define sno_GC_H

#include "sno_utility.h"

#define sno_GC_MARK_LIVE 1
#define sno_GC_MARK_DEAD 0

#define sno_START_GC_MEM 500000
#define sno_MIN_MEMORY_BEFORE_GC 10000000

typedef enum sno_GCStage {
	sno_GC_PAUSE,
	sno_GC_MARK,
	sno_GC_SWEEP,
} sno_GCStage;

void sno_full_gc(struct sno_State* state);

void sno_consider_gc(struct sno_State* state);

#endif
