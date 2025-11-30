#include "MEM_guardedalloc.h"

#include "LIB_math_base.h"
#include "LIB_math_solvers.h"

bool LIB_tridiagonal_solve(const float *a, const float *b, const float *c, const float *d, float *r_x, int count) {
	if (count < 1) {
		return false;
	}

	double *c1 = MEM_mallocN(sizeof(double) * count * 2, "tridiagonal_c1d1");
	if (!c1) {
		return false;
	}
	double *d1 = c1 + count;

	int i;
	double c_prev, d_prev, x_prev;

	/* forward pass */

	c1[0] = c_prev = ((double)c[0]) / b[0];
	d1[0] = d_prev = ((double)d[0]) / b[0];

	for (i = 1; i < count; i++) {
		double denum = b[i] - a[i] * c_prev;

		c1[i] = c_prev = c[i] / denum;
		d1[i] = d_prev = (d[i] - a[i] * d_prev) / denum;
	}

	/* back pass */

	x_prev = d_prev;
	r_x[--i] = (float)x_prev;

	while (--i >= 0) {
		x_prev = d1[i] - c1[i] * x_prev;
		r_x[i] = (float)x_prev;
	}

	MEM_freeN(c1);

	return isfinite(x_prev);
}

bool LIB_tridiagonal_solve_cyclic(const float *a, const float *b, const float *c, const float *d, float *r_x, int count) {
	if (count < 1) {
		return false;
	}

	/* Degenerate case not handled correctly by the generic formula. */
	if (count == 1) {
		r_x[0] = d[0] / (a[0] + b[0] + c[0]);

		return isfinite(r_x[0]);
	}

	/* Degenerate case that works but can be simplified. */
	if (count == 2) {
		const float a2[2] = {0, a[1] + c[1]};
		const float c2[2] = {a[0] + c[0], 0};

		return LIB_tridiagonal_solve(a2, b, c2, d, r_x, count);
	}

	/* If not really cyclic, fall back to the simple solver. */
	float a0 = a[0], cN = c[count - 1];

	if (a0 == 0.0f && cN == 0.0f) {
		return LIB_tridiagonal_solve(a, b, c, d, r_x, count);
	}

	size_t bytes = sizeof(float) * count;
	float *tmp = MEM_mallocN(sizeof(float) * count * 2, "tridiagonal_ex");
	if (!tmp) {
		return false;
	}
	float *b2 = tmp + count;

	/* Prepare the non-cyclic system; relies on tridiagonal_solve ignoring values. */
	memcpy(b2, b, bytes);
	b2[0] -= a0;
	b2[count - 1] -= cN;

	memset(tmp, 0, bytes);
	tmp[0] = a0;
	tmp[count - 1] = cN;

	/* solve for partial solution and adjustment vector */
	bool success = LIB_tridiagonal_solve(a, b2, c, tmp, tmp, count) && LIB_tridiagonal_solve(a, b2, c, d, r_x, count);

	/* apply adjustment */
	if (success) {
		float coeff = (r_x[0] + r_x[count - 1]) / (1.0f + tmp[0] + tmp[count - 1]);

		for (int i = 0; i < count; i++) {
			r_x[i] -= coeff * tmp[i];
		}
	}

	MEM_freeN(tmp);

	return success;
}
