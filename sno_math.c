#include "sno_math.h"

#include <math.h>

sno_Number sno_lerp(sno_Number a, sno_Number b, sno_Number t) {
	return a + (b - a) * t;
}

void sno_matrix_multiply(
	sno_Number* sno_restrict result,
	sno_Number* lhs,
	sno_Number* rhs,
	size_t lhs_rows,
	size_t lhs_columns,
	size_t rhs_rows,
	size_t rhs_columns
) {
	sno_assert(lhs_columns == rhs_rows);
	for (size_t j = 0; j < rhs_columns; j++) {
		for (size_t i = 0; i < lhs_rows; i++) {
			result[i + j * lhs_rows] = 0;
			for (size_t k = 0; k < lhs_columns; k++) {
				result[i + j * lhs_rows] += lhs[i + k * lhs_rows] * rhs[k + j * rhs_rows];
			}
		}
	}
}

