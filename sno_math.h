#ifndef sno_LINALG_H
#define sno_LINALG_H

#include "sno_utility.h"

sno_Number sno_lerp(sno_Number a, sno_Number b, sno_Number t);

void sno_matrix_multiply(
	sno_Number* sno_restrict result,
	sno_Number* lhs,
	sno_Number* rhs,
	size_t lhs_rows,
	size_t lhs_columns,
	size_t rhs_rows,
	size_t rhs_columns
);

#endif
