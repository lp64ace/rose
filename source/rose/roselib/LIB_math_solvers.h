#ifndef LIB_MATH_SOLVERS_H
#define LIB_MATH_SOLVERS_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Simple Solvers
 * \{ */

/**
 * \brief Solve a tridiagonal system of equations:
 *
 * a[i] * r_x[i-1] + b[i] * r_x[i] + c[i] * r_x[i+1] = d[i]
 *
 * Ignores a[0] and c[count-1]. Uses the Thomas algorithm, e.g. see wiki.
 *
 * \param r_x: output vector, may be shared with any of the input ones
 * \return true if success
 */
bool LIB_tridiagonal_solve(const float *a, const float *b, const float *c, const float *d, float *r_x, int count);
/**
 * \brief Solve a possibly cyclic tridiagonal system using the Sherman-Morrison formula.
 *
 * \param r_x: output vector, may be shared with any of the input ones
 * \return true if success
 */
bool LIB_tridiagonal_solve_cyclic(const float *a, const float *b, const float *c, const float *d, float *r_x, int count);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// LIB_MATH_SOLVERS_H
