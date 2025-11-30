#include "MEM_guardedalloc.h"

#include "RNA_access.h"

#include "LIB_easing.h"
#include "LIB_math_base.h"
#include "LIB_math_solvers.h"
#include "LIB_math_vector.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_fcurve.h"

#define SMALL -1.0e-10

/* -------------------------------------------------------------------- */
/** \name F-Curve
 * \{ */

FCurve *KER_fcurve_new(void) {
	FCurve *curve = MEM_mallocN(sizeof(FCurve), "FCurve");
	memset(curve, 0, sizeof(FCurve));
	return curve;
}

void KER_fcurves_free(ListBase *list) {
	LISTBASE_FOREACH_MUTABLE(FCurve *, fcurve, list) {
		KER_fcurve_free(fcurve);
	}
	LIB_listbase_clear(list);
}

ROSE_STATIC void fcurve_bezt_free(FCurve *fcurve) {
	if (fcurve == NULL) {
		return;
	}
	MEM_SAFE_FREE(fcurve->bezt);
	fcurve->totvert = 0;
}

void KER_fcurve_free(FCurve *fcurve) {
	if (fcurve == NULL) {
		return;
	}

	MEM_SAFE_FREE(fcurve->bezt);
	MEM_SAFE_FREE(fcurve->fpt);
	MEM_SAFE_FREE(fcurve->path);

	MEM_freeN(fcurve);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Finding Keyframes/Extents
 * \{ */

ROSE_INLINE int KER_fcurve_bezt_binarysearch_index_ex(const BezTriple *array, const float frame, const int length, const float threshold, bool *r_replace) {
	int start = 0, end = length;
	int loopbreaker = 0, maxloop = length * 2;

	/* Initialize replace-flag first. */
	*r_replace = false;

	/* Sneaky optimizations (don't go through searching process if...):
	 * - Keyframe to be added is to be added out of current bounds.
	 * - Keyframe to be added would replace one of the existing ones on bounds.
	 */
	if (length <= 0 || array == NULL) {
		return 0;
	}

	/* Check whether to add before/after/on. */
	/* 'First' Keyframe (when only one keyframe, this case is used) */
	float framenum = array[0].vec[1][0];
	if (IS_EQT(frame, framenum, threshold)) {
		*r_replace = true;
		return 0;
	}
	if (frame < framenum) {
		return 0;
	}

	/**
	 * Most of the time, this loop is just to find where to put it
	 * 'loopbreaker' is just here to prevent infinite loops.
	 */
	for (loopbreaker = 0; (start <= end) && (loopbreaker < maxloop); loopbreaker++) {
		const int mid = start + ((end - start) / 2);

		const float midfra = array[mid].vec[1][0];

		/* Check if exactly equal to midpoint. */
		if (IS_EQT(frame, midfra, threshold)) {
			*r_replace = true;
			return mid;
		}

		/* Repeat in upper/lower half. */
		if (frame > midfra) {
			start = mid + 1;
		}
		else if (frame < midfra) {
			end = mid - 1;
		}
	}

	/* Not found, so return where to place it. */
	return start;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Calculations
 * \{ */

void KER_fcurve_correct_bezpart(const float v1[2], float v2[2], float v3[2], const float v4[2]) {
	float h1[2], h2[2], len1, len2, len, fac;

	/* Calculate handle deltas. */
	h1[0] = v1[0] - v2[0];
	h1[1] = v1[1] - v2[1];

	h2[0] = v4[0] - v3[0];
	h2[1] = v4[1] - v3[1];

	/* Calculate distances:
	 * - len  = Span of time between keyframes.
	 * - len1 = Length of handle of start key.
	 * - len2 = Length of handle of end key.
	 */
	len = v4[0] - v1[0];
	len1 = fabsf(h1[0]);
	len2 = fabsf(h2[0]);

	/* If the handles have no length, no need to do any corrections. */
	if ((len1 + len2) == 0.0f) {
		return;
	}

	/* To prevent looping or rewinding, handles cannot
	 * exceed the adjacent key-frames time position. */
	if (len1 > len) {
		fac = len / len1;
		v2[0] = (v1[0] - fac * h1[0]);
		v2[1] = (v1[1] - fac * h1[1]);
	}

	if (len2 > len) {
		fac = len / len2;
		v3[0] = (v4[0] - fac * h2[0]);
		v3[1] = (v4[1] - fac * h2[1]);
	}
}

/**
 * Find roots of cubic equation (c0 + c1 x + c2 x^2 + c3 x^3)
 * \return number of roots in `o`.
 *
 * \note it is up to the caller to allocate enough memory for `o`.
 */
static int solve_cubic(double c0, double c1, double c2, double c3, float *o) {
	double a, b, c, p, q, d, t, phi;
	int nr = 0;

	if (c3 != 0.0) {
		a = c2 / c3;
		b = c1 / c3;
		c = c0 / c3;
		a = a / 3;

		p = b / 3 - a * a;
		q = (2 * a * a * a - a * b + c) / 2;
		d = q * q + p * p * p;

		if (d > 0.0) {
			t = sqrt(d);
			o[0] = (float)(sqrt3d(-q + t) + sqrt3d(-q - t) - a);

			if ((o[0] >= SMALL) && (o[0] <= 1.000001f)) {
				return 1;
			}
			return 0;
		}

		if (d == 0.0) {
			t = sqrt3d(-q);
			o[0] = (float)(2 * t - a);

			if ((o[0] >= SMALL) && (o[0] <= 1.000001f)) {
				nr++;
			}
			o[nr] = (float)(-t - a);

			if ((o[nr] >= SMALL) && (o[nr] <= 1.000001f)) {
				return nr + 1;
			}
			return nr;
		}

		phi = acos(-q / sqrt(-(p * p * p)));
		t = sqrt(-p);
		p = cos(phi / 3);
		q = sqrt(3 - 3 * p * p);
		o[0] = (float)(2 * t * p - a);

		if ((o[0] >= SMALL) && (o[0] <= 1.000001f)) {
			nr++;
		}
		o[nr] = (float)(-t * (p + q) - a);

		if ((o[nr] >= SMALL) && (o[nr] <= 1.000001f)) {
			nr++;
		}
		o[nr] = (float)(-t * (p - q) - a);

		if ((o[nr] >= SMALL) && (o[nr] <= 1.000001f)) {
			return nr + 1;
		}
		return nr;
	}
	a = c2;
	b = c1;
	c = c0;

	if (a != 0.0) {
		/* Discriminant */
		p = b * b - 4 * a * c;

		if (p > 0) {
			p = sqrt(p);
			o[0] = (float)((-b - p) / (2 * a));

			if ((o[0] >= SMALL) && (o[0] <= 1.000001f)) {
				nr++;
			}
			o[nr] = (float)((-b + p) / (2 * a));

			if ((o[nr] >= SMALL) && (o[nr] <= 1.000001f)) {
				return nr + 1;
			}
			return nr;
		}

		if (p == 0) {
			o[0] = (float)(-b / (2 * a));
			if ((o[0] >= SMALL) && (o[0] <= 1.000001f)) {
				return 1;
			}
		}

		return 0;
	}

	if (b != 0.0) {
		o[0] = (float)(-c / b);

		if ((o[0] >= SMALL) && (o[0] <= 1.000001f)) {
			return 1;
		}
		return 0;
	}

	if (c == 0.0) {
		o[0] = 0.0;
		return 1;
	}

	return 0;
}

/* Find root(s) ('zero') of a Bezier curve. */
ROSE_INLINE int findzero(float x, float q0, float q1, float q2, float q3, float *o) {
	const double c0 = q0 - x;
	const double c1 = 3.0f * (q1 - q0);
	const double c2 = 3.0f * (q0 - 2.0f * q1 + q2);
	const double c3 = q3 - q0 + 3.0f * (q1 - q2);

	return solve_cubic(c0, c1, c2, c3, o);
}

ROSE_INLINE void berekeny(float f1, float f2, float f3, float f4, float *o, int b) {
	float t, c0, c1, c2, c3;
	int a;

	c0 = f1;
	c1 = 3.0f * (f2 - f1);
	c2 = 3.0f * (f1 - 2.0f * f2 + f3);
	c3 = f4 - f1 + 3.0f * (f2 - f3);

	for (a = 0; a < b; a++) {
		t = o[a];
		o[a] = c0 + t * c1 + t * t * c2 + t * t * t * c3;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Evaluate
 * \{ */

ROSE_INLINE float fcurve_eval_keyframes_extrapolate(const FCurve *fcu, const BezTriple *bezts, float evaltime, int endpoint_offset, int direction_to_neighbor) {
	/* The first/last keyframe. */
	const BezTriple *endpoint_bezt = bezts + endpoint_offset;
	/* The second (to last) keyframe. */
	const BezTriple *neighbor_bezt = endpoint_bezt + direction_to_neighbor;

	if (endpoint_bezt->ipo == BEZT_IPO_CONST) {
		/* Constant (BEZT_IPO_HORIZ) extrapolation or constant interpolation, so just extend the
		 * endpoint's value. */
		return endpoint_bezt->vec[1][1];
	}

	if (endpoint_bezt->ipo == BEZT_IPO_LINEAR) {
		/* Use the next center point instead of our own handle for linear interpolated extrapolate. */
		if (fcu->totvert == 1) {
			return endpoint_bezt->vec[1][1];
		}

		const float dx = endpoint_bezt->vec[1][0] - evaltime;
		float fac = neighbor_bezt->vec[1][0] - endpoint_bezt->vec[1][0];

		/* Prevent division by zero. */
		if (fac == 0.0f) {
			return endpoint_bezt->vec[1][1];
		}

		fac = (neighbor_bezt->vec[1][1] - endpoint_bezt->vec[1][1]) / fac;
		return endpoint_bezt->vec[1][1] - (fac * dx);
	}

	/* Use the gradient of the second handle (later) of neighbor to calculate the gradient and thus
	 * the value of the curve at evaluation time. */
	const int handle = direction_to_neighbor > 0 ? 0 : 2;
	const float dx = endpoint_bezt->vec[1][0] - evaltime;
	float fac = endpoint_bezt->vec[1][0] - endpoint_bezt->vec[handle][0];

	/* Prevent division by zero. */
	if (fac == 0.0f) {
		return endpoint_bezt->vec[1][1];
	}

	fac = (endpoint_bezt->vec[1][1] - endpoint_bezt->vec[handle][1]) / fac;
	return endpoint_bezt->vec[1][1] - (fac * dx);
}

static float fcurve_eval_keyframes_interpolate(const FCurve *fcu, const BezTriple *bezts, float evaltime) {
	const float eps = 1.e-8f;
	uint a;

	/* Evaluation-time occurs somewhere in the middle of the curve. */
	bool exact = false;

	/* Use binary search to find appropriate keyframes...
	 *
	 * The threshold here has the following constraints:
	 * - 0.001 is too coarse:
	 *   We get artifacts with 2cm driver movements at 1BU = 1m.
	 *
	 * - 0.00001 is too fine:
	 *   Weird errors, like selecting the wrong keyframe range, occur.
	 */
	a = KER_fcurve_bezt_binarysearch_index_ex(bezts, evaltime, fcu->totvert, 0.0001, &exact);
	const BezTriple *bezt = bezts + a;

	if (exact) {
		/* Index returned must be interpreted differently when it sits on top of an existing keyframe
		 * - That keyframe is the start of the segment we need (see action_bug_2.blend in #39207).
		 */
		return bezt->vec[1][1];
	}

	/* Index returned refers to the keyframe that the eval-time occurs *before*
	 * - hence, that keyframe marks the start of the segment we're dealing with.
	 */
	const BezTriple *prevbezt = (a > 0) ? (bezt - 1) : bezt;

	/* Use if the key is directly on the frame, in rare cases this is needed else we get 0.0 instead.
	 * XXX: consult #39207 for examples of files where failure of these checks can cause issues. */
	if (fabsf(bezt->vec[1][0] - evaltime) < eps) {
		return bezt->vec[1][1];
	}

	if (evaltime < prevbezt->vec[1][0] || bezt->vec[1][0] < evaltime) {
		return 0.0f;
	}

	/* Evaluation-time occurs within the interval defined by these two keyframes. */
	const float begin = prevbezt->vec[1][1];
	const float change = bezt->vec[1][1] - prevbezt->vec[1][1];
	const float duration = bezt->vec[1][0] - prevbezt->vec[1][0];
	const float time = evaltime - prevbezt->vec[1][0];
	const float period = prevbezt->period;

	/* Value depends on interpolation mode. */
	if ((prevbezt->ipo == BEZT_IPO_CONST) || (duration == 0)) {
		/* Constant (evaltime not relevant, so no interpolation needed). */
		return prevbezt->vec[1][1];
	}

	switch (prevbezt->ipo) {
		/* Interpolation ...................................... */
		case BEZT_IPO_BEZ: {
			float v1[2], v2[2], v3[2], v4[2], opl[32];

			/* Bezier interpolation. */
			/* (v1, v2) are the first keyframe and its 2nd handle. */
			v1[0] = prevbezt->vec[1][0];
			v1[1] = prevbezt->vec[1][1];
			v2[0] = prevbezt->vec[2][0];
			v2[1] = prevbezt->vec[2][1];
			/* (v3, v4) are the last keyframe's 1st handle + the last keyframe. */
			v3[0] = bezt->vec[0][0];
			v3[1] = bezt->vec[0][1];
			v4[0] = bezt->vec[1][0];
			v4[1] = bezt->vec[1][1];

			if (fabsf(v1[1] - v4[1]) < FLT_EPSILON && fabsf(v2[1] - v3[1]) < FLT_EPSILON && fabsf(v3[1] - v4[1]) < FLT_EPSILON) {
				/* Optimization: If all the handles are flat/at the same values,
				 * the value is simply the shared value (see #40372 -> F91346).
				 */
				return v1[1];
			}
			/* Adjust handles so that they don't overlap (forming a loop). */
			KER_fcurve_correct_bezpart(v1, v2, v3, v4);

			/* Try to get a value for this position - if failure, try another set of points. */
			if (!findzero(evaltime, v1[0], v2[0], v3[0], v4[0], opl)) {
				return 0.0;
			}

			berekeny(v1[1], v2[1], v3[1], v4[1], opl, 1);
			return opl[0];
		}
		case BEZT_IPO_LINEAR:
			/* Linear - simply linearly interpolate between values of the two keyframes. */
			return LIB_easing_linear_ease(time, begin, change, duration);

		default:
			ROSE_assert_unreachable();

			return prevbezt->vec[1][1];
	}

	return 0.0f;
}

/* Calculate F-Curve value for 'evaltime' using #BezTriple keyframes. */
ROSE_INLINE float fcurve_eval_keyframes(const FCurve *fcu, const BezTriple *bezts, float evaltime) {
	if (evaltime <= bezts->vec[1][0]) {
		return fcurve_eval_keyframes_extrapolate(fcu, bezts, evaltime, 0, +1);
	}

	const BezTriple *lastbezt = bezts + fcu->totvert - 1;
	if (lastbezt->vec[1][0] <= evaltime) {
		return fcurve_eval_keyframes_extrapolate(fcu, bezts, evaltime, fcu->totvert - 1, -1);
	}

	return fcurve_eval_keyframes_interpolate(fcu, bezts, evaltime);
}

/* Calculate F-Curve value for 'evaltime' using #FPoint samples. */
ROSE_INLINE float fcurve_eval_samples(const FCurve *fcu, const FPoint *fpts, float evaltime) {
	float cvalue = 0.0f;

	/* Get pointers. */
	const FPoint *prevfpt = fpts;
	const FPoint *lastfpt = prevfpt + fcu->totvert - 1;

	/* Evaluation time at or past endpoints? */
	if (prevfpt->vec[0] >= evaltime) {
		/* Before or on first sample, so just extend value. */
		cvalue = prevfpt->vec[1];
	}
	else if (lastfpt->vec[0] <= evaltime) {
		/* After or on last sample, so just extend value. */
		cvalue = lastfpt->vec[1];
	}
	else {
		float t = fabsf(evaltime - floorf(evaltime));

		/* Find the one on the right frame (assume that these are spaced on 1-frame intervals). */
		const FPoint *fpt = prevfpt + ((int)evaltime - (int)prevfpt->vec[0]);

		/* If not exactly on the frame, perform linear interpolation with the next one. */
		if (t != 0.0f && t < 1.0f) {
			cvalue = interpf(fpt->vec[1], (fpt + 1)->vec[1], 1.0f - t);
		}
		else {
			cvalue = fpt->vec[1];
		}
	}

	return cvalue;
}

ROSE_INLINE float fcurve_evaluate_ex(FCurve *fcurve, float ctime, float cvalue) {
	if (fcurve->bezt) {
		cvalue = fcurve_eval_keyframes(fcurve, fcurve->bezt, ctime);
	}
	else if (fcurve->fpt) {
		cvalue = fcurve_eval_samples(fcurve, fcurve->fpt, ctime);
	}

	return cvalue;
}

ROSE_INLINE float fcurve_evaluate(FCurve *fcurve, float ctime) {
	return fcurve_evaluate_ex(fcurve, ctime, 0.0f);
}

bool KER_fcurve_is_empty(FCurve *fcurve) {
	return fcurve->totvert == 0;
}

float KER_fcurve_evaluate(PathResolvedRNA *rna, FCurve *fcurve, float ctime) {
	if (KER_fcurve_is_empty(fcurve)) {
		return 0.0f;
	}

	return fcurve_evaluate(fcurve, ctime);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Edit
 * \{ */

void KER_fcurve_bezt_resize(FCurve *fcurve, int totvert) {
	ROSE_assert(totvert >= 0);

	if (totvert == 0) {
		fcurve_bezt_free(fcurve);
		return;
	}

	fcurve->bezt = MEM_reallocN(fcurve->bezt, sizeof(*(fcurve->bezt)) * totvert);

	/* Zero out all the newly-allocated beztriples. This is necessary, as it is likely that only some
	 * of the fields will actually be updated by the caller. */
	const int oldvert = fcurve->totvert;
	if (totvert > oldvert) {
		memset(&fcurve->bezt[oldvert], 0, sizeof(fcurve->bezt[0]) * (totvert - oldvert));
	}

	fcurve->totvert = totvert;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name F-Curve Sanity
 * \{ */

void KER_nurb_handle_calc(BezTriple *bezt, BezTriple *prev, BezTriple *next, bool fcurve) {
	KER_nurb_handle_calc_ex(bezt, prev, next, BEZT_FLAG_SELECT, fcurve);
}

void KER_nurb_handle_calc_ex(BezTriple *bezt, BezTriple *prev, BezTriple *next, int flag, bool is_fcurve) {
#define p2_h1 ((p2) - 3)
#define p2_h2 ((p2) + 3)

	const float *p1, *p3;
	float *p2;
	float pt[3];
	float dvec_a[3], dvec_b[3];
	float len, len_a, len_b;
	const float eps = 1e-5;

	/* assume normal handle until we check */
	bezt->type = HD_AUTOTYPE_NORMAL;

	if (bezt->h1 == 0 && bezt->h2 == 0) {
		return;
	}

	p2 = bezt->vec[1];

	if (prev == NULL) {
		p3 = next->vec[1];
		pt[0] = 2.0f * p2[0] - p3[0];
		pt[1] = 2.0f * p2[1] - p3[1];
		pt[2] = 2.0f * p2[2] - p3[2];
		p1 = pt;
	}
	else {
		p1 = prev->vec[1];
	}

	if (next == NULL) {
		pt[0] = 2.0f * p2[0] - p1[0];
		pt[1] = 2.0f * p2[1] - p1[1];
		pt[2] = 2.0f * p2[2] - p1[2];
		p3 = pt;
	}
	else {
		p3 = next->vec[1];
	}

	sub_v3_v3v3(dvec_a, p2, p1);
	sub_v3_v3v3(dvec_b, p3, p2);

	if (is_fcurve) {
		len_a = dvec_a[0];
		len_b = dvec_b[0];
	}
	else {
		len_a = len_v3(dvec_a);
		len_b = len_v3(dvec_b);
	}

	if (len_a == 0.0f) {
		len_a = 1.0f;
	}
	if (len_b == 0.0f) {
		len_b = 1.0f;
	}

	if (ELEM(bezt->h1, HD_AUTO, HD_AUTO_ANIM) || ELEM(bezt->h2, HD_AUTO, HD_AUTO_ANIM)) { /* auto */
		float tvec[3];
		tvec[0] = dvec_b[0] / len_b + dvec_a[0] / len_a;
		tvec[1] = dvec_b[1] / len_b + dvec_a[1] / len_a;
		tvec[2] = dvec_b[2] / len_b + dvec_a[2] / len_a;

		if (is_fcurve) {
			/* force the horizontal handle size to be 1/3 of the key interval so that
			 * the X component of the parametric bezier curve is a linear spline */
			len = 6.0f / 2.5614f;
		}
		else {
			len = len_v3(tvec);
		}
		len *= 2.5614f;

		if (len != 0.0f) {
			/* Only for F-Curves. */
			bool leftviolate = false, rightviolate = false;

			if (!is_fcurve) {
				len_a = ROSE_MIN(len_a, 5.0f * len_b);
				len_b = ROSE_MIN(len_b, 5.0f * len_a);
			}

			if (ELEM(bezt->h1, HD_AUTO, HD_AUTO_ANIM)) {
				len_a /= len;
				madd_v3_v3v3fl(p2_h1, p2, tvec, -len_a);

				if ((bezt->h1 == HD_AUTO_ANIM) && next && prev) { /* keep horizontal if extrema */
					float ydiff1 = prev->vec[1][1] - bezt->vec[1][1];
					float ydiff2 = next->vec[1][1] - bezt->vec[1][1];
					if ((ydiff1 <= 0.0f && ydiff2 <= 0.0f) || (ydiff1 >= 0.0f && ydiff2 >= 0.0f)) {
						bezt->vec[0][1] = bezt->vec[1][1];
						bezt->type = HD_AUTOTYPE_LOCKED_FINAL;
					}
					else { /* handles should not be beyond y coord of two others */
						if (ydiff1 <= 0.0f) {
							if (prev->vec[1][1] > bezt->vec[0][1]) {
								bezt->vec[0][1] = prev->vec[1][1];
								leftviolate = true;
							}
						}
						else {
							if (prev->vec[1][1] < bezt->vec[0][1]) {
								bezt->vec[0][1] = prev->vec[1][1];
								leftviolate = true;
							}
						}
					}
				}
			}
			if (ELEM(bezt->h2, HD_AUTO, HD_AUTO_ANIM)) {
				len_b /= len;
				madd_v3_v3v3fl(p2_h2, p2, tvec, len_b);

				if ((bezt->h2 == HD_AUTO_ANIM) && next && prev) { /* keep horizontal if extrema */
					float ydiff1 = prev->vec[1][1] - bezt->vec[1][1];
					float ydiff2 = next->vec[1][1] - bezt->vec[1][1];
					if ((ydiff1 <= 0.0f && ydiff2 <= 0.0f) || (ydiff1 >= 0.0f && ydiff2 >= 0.0f)) {
						bezt->vec[2][1] = bezt->vec[1][1];
						bezt->type = HD_AUTOTYPE_LOCKED_FINAL;
					}
					else { /* handles should not be beyond y coord of two others */
						if (ydiff1 <= 0.0f) {
							if (next->vec[1][1] < bezt->vec[2][1]) {
								bezt->vec[2][1] = next->vec[1][1];
								rightviolate = true;
							}
						}
						else {
							if (next->vec[1][1] > bezt->vec[2][1]) {
								bezt->vec[2][1] = next->vec[1][1];
								rightviolate = true;
							}
						}
					}
				}
			}
			if (leftviolate || rightviolate) { /* align left handle */
				ROSE_assert(is_fcurve);
				/* simple 2d calculation */
				float h1_x = p2_h1[0] - p2[0];
				float h2_x = p2[0] - p2_h2[0];

				if (leftviolate) {
					p2_h2[1] = p2[1] + ((p2[1] - p2_h1[1]) / h1_x) * h2_x;
				}
				else {
					p2_h1[1] = p2[1] + ((p2[1] - p2_h2[1]) / h2_x) * h1_x;
				}
			}
		}
	}

	if (/* When one handle is free, aligning makes no sense, see: #35952 */
		ELEM(HD_FREE, bezt->h1, bezt->h2) ||
		/* Also when no handles are aligned, skip this step. */
		(!ELEM(HD_ALIGN, bezt->h1, bezt->h2) && !ELEM(HD_ALIGN_DOUBLESIDE, bezt->h1, bezt->h2))) {
		/* Handles need to be updated during animation and applying stuff like hooks,
		 * but in such situations it's quite difficult to distinguish in which order
		 * align handles should be aligned so skip them for now. */
		return;
	}

	len_a = len_v3v3(p2, p2_h1);
	len_b = len_v3v3(p2, p2_h2);

	if (len_a == 0.0f) {
		len_a = 1.0f;
	}
	if (len_b == 0.0f) {
		len_b = 1.0f;
	}

	const float len_ratio = len_a / len_b;

	if (bezt->f1 & flag) {						 /* order of calculation */
		if (ELEM(bezt->h2, HD_ALIGN, HD_ALIGN_DOUBLESIDE)) { /* aligned */
			if (len_a > eps) {
				len = 1.0f / len_ratio;
				p2_h2[0] = p2[0] + len * (p2[0] - p2_h1[0]);
				p2_h2[1] = p2[1] + len * (p2[1] - p2_h1[1]);
				p2_h2[2] = p2[2] + len * (p2[2] - p2_h1[2]);
			}
		}
		if (ELEM(bezt->h1, HD_ALIGN, HD_ALIGN_DOUBLESIDE)) {
			if (len_b > eps) {
				len = len_ratio;
				p2_h1[0] = p2[0] + len * (p2[0] - p2_h2[0]);
				p2_h1[1] = p2[1] + len * (p2[1] - p2_h2[1]);
				p2_h1[2] = p2[2] + len * (p2[2] - p2_h2[2]);
			}
		}
	}
	else {
		if (ELEM(bezt->h1, HD_ALIGN, HD_ALIGN_DOUBLESIDE)) {
			if (len_b > eps) {
				len = len_ratio;
				p2_h1[0] = p2[0] + len * (p2[0] - p2_h2[0]);
				p2_h1[1] = p2[1] + len * (p2[1] - p2_h2[1]);
				p2_h1[2] = p2[2] + len * (p2[2] - p2_h2[2]);
			}
		}
		if (ELEM(bezt->h2, HD_ALIGN, HD_ALIGN_DOUBLESIDE)) { /* aligned */
			if (len_a > eps) {
				len = 1.0f / len_ratio;
				p2_h2[0] = p2[0] + len * (p2[0] - p2_h1[0]);
				p2_h2[1] = p2[1] + len * (p2[1] - p2_h1[1]);
				p2_h2[2] = p2[2] + len * (p2[2] - p2_h1[2]);
			}
		}
	}

#undef p2_h1
#undef p2_h2
}

/**
 * A utility function for allocating a number of arrays of the same length
 * with easy error checking and de-allocation, and an easy way to add or remove
 * arrays that are processed in this way when changing code.
 *
 * floats, chars: null-terminated arrays of pointers to array pointers that need to be
 * allocated.
 *
 * Returns: pointer to the buffer that contains all of the arrays.
 */
ROSE_INLINE void *allocate_arrays(int count, float ***floats, char ***chars, const char *name) {
	size_t num_floats = 0, num_chars = 0;

	while (floats && floats[num_floats]) {
		num_floats++;
	}

	while (chars && chars[num_chars]) {
		num_chars++;
	}

	void *buffer = MEM_mallocN((sizeof(float) * num_floats + num_chars) * count, name);

	if (!buffer) {
		return NULL;
	}

	float *fptr = (float *)buffer;

	for (int i = 0; i < num_floats; i++, fptr += count) {
		*floats[i] = fptr;
	}

	char *cptr = (char *)fptr;

	for (int i = 0; i < num_chars; i++, cptr += count) {
		*chars[i] = cptr;
	}

	return buffer;
}

ROSE_INLINE void free_arrays(void *buffer) {
	MEM_freeN(buffer);
}

/* computes in which direction to change h[i] to satisfy conditions better */
ROSE_INLINE float bezier_relax_direction(const float *a, const float *b, const float *c, const float *d, const float *h, int i, int count) {
	/* current deviation between sides of the equation */
	float state = a[i] * h[(i + count - 1) % count] + b[i] * h[i] + c[i] * h[(i + 1) % count] - d[i];

	/* only the sign is meaningful */
	return -state * b[i];
}

ROSE_INLINE void bezier_lock_unknown(float *a, float *b, float *c, float *d, int i, float value) {
	a[i] = c[i] = 0.0f;
	b[i] = 1.0f;
	d[i] = value;
}

ROSE_INLINE void bezier_restore_equation(float *a, float *b, float *c, float *d, const float *a0, const float *b0, const float *c0, const float *d0, int i) {
	a[i] = a0[i];
	b[i] = b0[i];
	c[i] = c0[i];
	d[i] = d0[i];
}

ROSE_INLINE bool tridiagonal_solve_with_limits(float *a, float *b, float *c, float *d, float *h, const float *hmin, const float *hmax, int solve_count) {
	float *a0, *b0, *c0, *d0;
	float **arrays[] = {&a0, &b0, &c0, &d0, NULL};
	char *is_locked, *num_unlocks;
	char **flagarrays[] = {&is_locked, &num_unlocks, NULL};

	void *tmps = allocate_arrays(solve_count, arrays, flagarrays, "tridiagonal_solve_with_limits");
	if (!tmps) {
		return false;
	}

	memcpy(a0, a, sizeof(float) * solve_count);
	memcpy(b0, b, sizeof(float) * solve_count);
	memcpy(c0, c, sizeof(float) * solve_count);
	memcpy(d0, d, sizeof(float) * solve_count);

	memset(is_locked, 0, solve_count);
	memset(num_unlocks, 0, solve_count);

	bool overshoot, unlocked;

	do {
		if (!LIB_tridiagonal_solve_cyclic(a, b, c, d, h, solve_count)) {
			free_arrays(tmps);
			return false;
		}

		/* first check if any handles overshoot the limits, and lock them */
		bool all = false, locked = false;

		overshoot = unlocked = false;

		do {
			for (int i = 0; i < solve_count; i++) {
				if (h[i] >= hmin[i] && h[i] <= hmax[i]) {
					continue;
				}

				overshoot = true;

				float target = h[i] > hmax[i] ? hmax[i] : hmin[i];

				/* heuristically only lock handles that go in the right direction if there are such ones */
				if (target != 0.0f || all) {
					/* mark item locked */
					is_locked[i] = 1;

					bezier_lock_unknown(a, b, c, d, i, target);
					locked = true;
				}
			}

			all = true;
		} while (overshoot && !locked);

		/* If no handles overshot and were locked,
		 * see if it may be a good idea to unlock some handles. */
		if (!locked) {
			for (int i = 0; i < solve_count; i++) {
				/* to definitely avoid infinite loops limit this to 2 times */
				if (!is_locked[i] || num_unlocks[i] >= 2) {
					continue;
				}

				/* if the handle wants to move in allowable direction, release it */
				float relax = bezier_relax_direction(a0, b0, c0, d0, h, i, solve_count);

				if ((relax > 0 && h[i] < hmax[i]) || (relax < 0 && h[i] > hmin[i])) {
					bezier_restore_equation(a, b, c, d, a0, b0, c0, d0, i);

					is_locked[i] = 0;
					num_unlocks[i]++;
					unlocked = true;
				}
			}
		}
	} while (overshoot || unlocked);

	free_arrays(tmps);
	return true;
}

/*
 * This function computes the handles of a series of auto bezier points
 * on the basis of 'no acceleration discontinuities' at the points.
 * The first and last bezier points are considered 'fixed' (their handles are not touched)
 * The result is the smoothest possible trajectory going through intermediate points.
 * The difficulty is that the handles depends on their neighbors.
 *
 * The exact solution is found by solving a tridiagonal matrix equation formed
 * by the continuity and boundary conditions. Although theoretically handle position
 * is affected by all other points of the curve segment, in practice the influence
 * decreases exponentially with distance.
 *
 * NOTE: this algorithm assumes that the handle horizontal size is always 1/3 of the
 * of the interval to the next point. This rule ensures linear interpolation of time.
 *
 * ^ height (co 1)
 * |                                            yN
 * |                                   yN-1     |
 * |                      y2           |        |
 * |           y1         |            |        |
 * |    y0     |          |            |        |
 * |    |      |          |            |        |
 * |    |      |          |            |        |
 * |    |      |          |            |        |
 * |------dx1--------dx2--------- ~ -------dxN-------------------> time (co 0)
 *
 * Notation:
 *
 *   x[i], y[i] - keyframe coordinates
 *   h[i]       - right handle y offset from y[i]
 *
 *   dx[i] = x[i] - x[i-1]
 *   dy[i] = y[i] - y[i-1]
 *
 * Mathematical basis:
 *
 * 1. Handle lengths on either side of each point are connected by a factor
 *    ensuring continuity of the first derivative:
 *
 *    l[i] = dx[i+1]/dx[i]
 *
 * 2. The tridiagonal system is formed by the following equation, which is derived
 *    by differentiating the bezier curve and specifies second derivative continuity
 *    at every point:
 *
 *    l[i]^2 * h[i-1] + (2*l[i]+2) * h[i] + 1/l[i+1] * h[i+1] = dy[i]*l[i]^2 + dy[i+1]
 *
 * 3. If this point is adjacent to a manually set handle with X size not equal to 1/3
 *    of the horizontal interval, this equation becomes slightly more complex:
 *
 *    l[i]^2 * h[i-1] + (3*(1-R[i-1])*l[i] + 3*(1-L[i+1])) * h[i] + 1/l[i+1] * h[i+1] = dy[i]*l[i]^2 + dy[i+1]
 *
 *    The difference between equations amounts to this, and it's obvious that when R[i-1]
 *    and L[i+1] are both 1/3, it becomes zero:
 *
 *    ( (1-3*R[i-1])*l[i] + (1-3*L[i+1]) ) * h[i]
 *
 * 4. The equations for zero acceleration border conditions are basically the above
 *    equation with parts omitted, so the handle size correction also applies.
 *
 * 5. The fully cyclic curve case is handled by eliminating one of the end points,
 *    and instead of border conditions connecting the curve via a set of equations:
 *
 *    l[0] = l[N] = dx[1] / dx[N]
 *    dy[0] = dy[N]
 *    Continuity equation (item 2) for i = 0.
 *    Substitute h[0] for h[N] and h[N-1] for h[-1]
 */

ROSE_INLINE void bezier_eq_continuous(float *a, float *b, float *c, float *d, const float *dy, const float *l, int i) {
	a[i] = l[i] * l[i];
	b[i] = 2.0f * (l[i] + 1);
	c[i] = 1.0f / l[i + 1];
	d[i] = dy[i] * l[i] * l[i] + dy[i + 1];
}

ROSE_INLINE void bezier_eq_noaccel_right(float *a, float *b, float *c, float *d, const float *dy, const float *l, int i) {
	a[i] = 0.0f;
	b[i] = 2.0f;
	c[i] = 1.0f / l[i + 1];
	d[i] = dy[i + 1];
}

ROSE_INLINE void bezier_eq_noaccel_left(float *a, float *b, float *c, float *d, const float *dy, const float *l, int i) {
	a[i] = l[i] * l[i];
	b[i] = 2.0f * l[i];
	c[i] = 0.0f;
	d[i] = dy[i] * l[i] * l[i];
}

/* auto clamp prevents its own point going the wrong way, and adjacent handles overshooting */
ROSE_INLINE void bezier_clamp(float *hmax, float *hmin, int i, float dy, bool no_reverse, bool no_overshoot) {
	if (dy > 0) {
		if (no_overshoot) {
			hmax[i] = ROSE_MIN(hmax[i], dy);
		}
		if (no_reverse) {
			hmin[i] = 0.0f;
		}
	}
	else if (dy < 0) {
		if (no_reverse) {
			hmax[i] = 0.0f;
		}
		if (no_overshoot) {
			hmin[i] = ROSE_MAX(hmin[i], dy);
		}
	}
	else if (no_reverse || no_overshoot) {
		hmax[i] = hmin[i] = 0.0f;
	}
}

/* write changes to a bezier handle */
ROSE_INLINE void bezier_output_handle_inner(BezTriple *bezt, bool right, const float newval[3], bool endpoint) {
	float tmp[3];

	int idx = right ? 2 : 0;
	char hr = right ? bezt->h2 : bezt->h1;
	char hm = right ? bezt->h1 : bezt->h2;

	/* only assign Auto/Vector handles */
	if (!ELEM(hr, HD_AUTO, HD_AUTO_ANIM)) {
		return;
	}

	copy_v3_v3(bezt->vec[idx], newval);

	/* fix up the Align handle if any */
	if (ELEM(hm, HD_ALIGN, HD_ALIGN_DOUBLESIDE)) {
		float hlen = len_v3v3(bezt->vec[1], bezt->vec[2 - idx]);
		float h2len = len_v3v3(bezt->vec[1], bezt->vec[idx]);

		sub_v3_v3v3(tmp, bezt->vec[1], bezt->vec[idx]);
		madd_v3_v3v3fl(bezt->vec[2 - idx], bezt->vec[1], tmp, hlen / h2len);
	}
	/* at end points of the curve, mirror handle to the other side */
	else if (endpoint && ELEM(hm, HD_AUTO, HD_AUTO_ANIM)) {
		sub_v3_v3v3(tmp, bezt->vec[1], bezt->vec[idx]);
		add_v3_v3v3(bezt->vec[2 - idx], bezt->vec[1], tmp);
	}
}

ROSE_INLINE void bezier_output_handle(BezTriple *bezt, bool right, float dy, bool endpoint) {
	float tmp[3];

	copy_v3_v3(tmp, bezt->vec[right ? 2 : 0]);

	tmp[1] = bezt->vec[1][1] + dy;

	bezier_output_handle_inner(bezt, right, tmp, endpoint);
}

ROSE_INLINE bool bezier_check_solve_end_handle(BezTriple *bezt, char htype, bool end) {
	return (end && ELEM(htype, HD_AUTO, HD_AUTO_ANIM) && bezt->type == HD_AUTOTYPE_NORMAL);
}

ROSE_INLINE float bezier_calc_handle_adj(float hsize[2], float dx) {
	/* if handles intersect in x direction, they are scaled to fit */
	float fac = dx / (hsize[0] + dx / 3.0f);
	if (fac < 1.0f) {
		mul_v2_fl(hsize, fac);
	}
	return 1.0f - 3.0f * hsize[0] / dx;
}

ROSE_INLINE void bezier_handle_calc_smooth_fcurve(BezTriple *bezt, int total, int start, int count, bool cycle) {
	float *dx, *dy, *l, *a, *b, *c, *d, *h, *hmax, *hmin;
	float **arrays[] = {&dx, &dy, &l, &a, &b, &c, &d, &h, &hmax, &hmin, NULL};

	int solve_count = count;

	/* verify index ranges */

	if (count < 2) {
		return;
	}

	ROSE_assert(start < total - 1 && count <= total);
	ROSE_assert(start + count <= total || cycle);

	bool full_cycle = (start == 0 && count == total && cycle);

	BezTriple *bezt_first = &bezt[start];
	BezTriple *bezt_last = &bezt[(start + count > total) ? start + count - total : start + count - 1];

	bool solve_first = bezier_check_solve_end_handle(bezt_first, bezt_first->h2, start == 0);
	bool solve_last = bezier_check_solve_end_handle(bezt_last, bezt_last->h1, start + count == total);

	if (count == 2 && !full_cycle && solve_first == solve_last) {
		return;
	}

	/* allocate all */

	void *tmp_buffer = allocate_arrays(count, arrays, NULL, "bezier_calc_smooth_tmp");
	if (!tmp_buffer) {
		return;
	}

	/* point locations */

	dx[0] = dy[0] = NAN;

	for (int i = 1, j = start + 1; i < count; i++, j++) {
		dx[i] = bezt[j].vec[1][0] - bezt[j - 1].vec[1][0];
		dy[i] = bezt[j].vec[1][1] - bezt[j - 1].vec[1][1];

		/* when cyclic, jump from last point to first */
		if (cycle && j == total - 1) {
			j = 0;
		}
	}

	/* ratio of x intervals */

	if (full_cycle) {
		dx[0] = dx[count - 1];
		dy[0] = dy[count - 1];

		l[0] = l[count - 1] = dx[1] / dx[0];
	}
	else {
		l[0] = l[count - 1] = 1.0f;
	}

	for (int i = 1; i < count - 1; i++) {
		l[i] = dx[i + 1] / dx[i];
	}

	/* compute handle clamp ranges */

	bool clamped_prev = false, clamped_cur = ELEM(HD_AUTO_ANIM, bezt_first->h1, bezt_first->h2);

	for (int i = 0; i < count; i++) {
		hmax[i] = FLT_MAX;
		hmin[i] = -FLT_MAX;
	}

	for (int i = 1, j = start + 1; i < count; i++, j++) {
		clamped_prev = clamped_cur;
		clamped_cur = ELEM(HD_AUTO_ANIM, bezt[j].h1, bezt[j].h2);

		if (cycle && j == total - 1) {
			j = 0;
			clamped_cur = clamped_cur || ELEM(HD_AUTO_ANIM, bezt[j].h1, bezt[j].h2);
		}

		bezier_clamp(hmax, hmin, i - 1, dy[i], clamped_prev, clamped_prev);
		bezier_clamp(hmax, hmin, i, dy[i] * l[i], clamped_cur, clamped_cur);
	}

	/* full cycle merges first and last points into continuous loop */

	float first_handle_adj = 0.0f, last_handle_adj = 0.0f;

	if (full_cycle) {
		/* reduce the number of unknowns by one */
		int i = solve_count = count - 1;

		hmin[0] = ROSE_MAX(hmin[0], hmin[i]);
		hmax[0] = ROSE_MIN(hmax[0], hmax[i]);

		solve_first = solve_last = true;

		bezier_eq_continuous(a, b, c, d, dy, l, 0);
	}
	else {
		float tmp[2];

		/* boundary condition: fixed handles or zero curvature */
		if (!solve_first) {
			sub_v2_v2v2(tmp, bezt_first->vec[2], bezt_first->vec[1]);
			first_handle_adj = bezier_calc_handle_adj(tmp, dx[1]);

			bezier_lock_unknown(a, b, c, d, 0, tmp[1]);
		}
		else {
			bezier_eq_noaccel_right(a, b, c, d, dy, l, 0);
		}

		if (!solve_last) {
			sub_v2_v2v2(tmp, bezt_last->vec[1], bezt_last->vec[0]);
			last_handle_adj = bezier_calc_handle_adj(tmp, dx[count - 1]);

			bezier_lock_unknown(a, b, c, d, count - 1, tmp[1]);
		}
		else {
			bezier_eq_noaccel_left(a, b, c, d, dy, l, count - 1);
		}
	}

	/* main tridiagonal system of equations */

	for (int i = 1; i < count - 1; i++) {
		bezier_eq_continuous(a, b, c, d, dy, l, i);
	}

	/* apply correction for user-defined handles with nonstandard x positions */

	if (!full_cycle) {
		if (count > 2 || solve_last) {
			b[1] += l[1] * first_handle_adj;
		}

		if (count > 2 || solve_first) {
			b[count - 2] += last_handle_adj;
		}
	}

	/* solve and output results */

	if (tridiagonal_solve_with_limits(a, b, c, d, h, hmin, hmax, solve_count)) {
		if (full_cycle) {
			h[count - 1] = h[0];
		}

		for (int i = 1, j = start + 1; i < count - 1; i++, j++) {
			bool end = (j == total - 1);

			bezier_output_handle(&bezt[j], false, -h[i] / l[i], end);

			if (end) {
				j = 0;
			}

			bezier_output_handle(&bezt[j], true, h[i], end);
		}

		if (solve_first) {
			bezier_output_handle(bezt_first, true, h[0], start == 0);
		}

		if (solve_last) {
			bezier_output_handle(bezt_last, false, -h[count - 1] / l[count - 1], start + count == total);
		}
	}

	/* free all */

	free_arrays(tmp_buffer);
}

ROSE_INLINE bool is_free_auto_point(BezTriple *bezt) {
	return BEZT_IS_AUTOH(bezt) && bezt->type == HD_AUTOTYPE_NORMAL;
}

void KER_nurb_handle_smooth_fcurve(BezTriple *bezt, int total, bool cyclic) {
	/* ignore cyclic extrapolation if end points are locked */
	cyclic = cyclic && is_free_auto_point(&bezt[0]) && is_free_auto_point(&bezt[total - 1]);

	/* if cyclic, try to find a sequence break point */
	int search_base = 0;

	if (cyclic) {
		for (int i = 1; i < total - 1; i++) {
			if (!is_free_auto_point(&bezt[i])) {
				search_base = i;
				break;
			}
		}

		/* all points of the curve are freely changeable auto handles - solve as full cycle */
		if (search_base == 0) {
			bezier_handle_calc_smooth_fcurve(bezt, total, 0, total, cyclic);
			return;
		}
	}

	/* Find continuous sub-sequences of free auto handles and smooth them, starting at search_base.
	 * In cyclic mode these sub-sequences can span the cycle boundary. */
	int start = search_base, count = 1;

	for (int i = 1, j = start + 1; i < total; i++, j++) {
		/* in cyclic mode: jump from last to first point when necessary */
		if (j == total - 1 && cyclic) {
			j = 0;
		}

		/* non auto handle closes the list (we come here at least for the last handle, see above) */
		if (!is_free_auto_point(&bezt[j])) {
			bezier_handle_calc_smooth_fcurve(bezt, total, start, count + 1, cyclic);
			start = j;
			count = 1;
		}
		else {
			count++;
		}
	}

	if (count > 1) {
		bezier_handle_calc_smooth_fcurve(bezt, total, start, count, cyclic);
	}
}

ROSE_INLINE BezTriple *cycle_offset_triple(bool cycle, BezTriple *out, const BezTriple *in, const BezTriple *from, const BezTriple *to) {
	if (!cycle) {
		return NULL;
	}

	memcpy(out, in, sizeof(BezTriple));

	float delta[3];
	sub_v3_v3v3(delta, to->vec[1], from->vec[1]);

	for (int i = 0; i < 3; i++) {
		add_v3_v3(out->vec[i], delta);
	}

	return out;
}

void KER_fcurve_handles_recalc(FCurve *fcu) {
	KER_fcurve_handles_recalc_ex(fcu, BEZT_FLAG_SELECT);
}

void KER_fcurve_handles_recalc_ex(FCurve *fcu, int flag) {
	if (ELEM(NULL, fcu, fcu->bezt) || (fcu->totvert < 2)) {
		return;
	}

	/* If the first modifier is Cycles, smooth the curve through the cycle. */
	BezTriple *first = &fcu->bezt[0];
	BezTriple *last = &fcu->bezt[fcu->totvert - 1];

	const bool cycle = false;

	BezTriple tmp;
	for (size_t i = 0; i < fcu->totvert; i++) {
		BezTriple *bezt = &fcu->bezt[i];
		BezTriple *prev = NULL;
		BezTriple *next = NULL;

		if (i > 0) {
			prev = (bezt - 1);
		}
		else {
			prev = cycle_offset_triple(cycle, &tmp, &fcu->bezt[fcu->totvert - 2], last, first);
		}
		if (i < fcu->totvert - 1) {
			next = (bezt + 1);
		}
		else {
			next = cycle_offset_triple(cycle, &tmp, &fcu->bezt[1], first, last);
		}

		/**
		 * Clamp timing of handles to be on either side of beztriple. The threshold with
		 * increment/decrement ulp ensures that the handle length doesn't reach 0 at which point
		 * there would be no way to ensure that handles stay aligned. This adds an issue where if a
		 * handle is scaled to 0, the other side is set to be horizontal.
		 * See #141029 for more info.
		 */
		const float threshold = 1e-3f;
		CLAMP_MAX(bezt->vec[0][0], decrement_ulp(bezt->vec[1][0] - threshold));
		CLAMP_MIN(bezt->vec[2][0], increment_ulp(bezt->vec[1][0] + threshold));

		KER_nurb_handle_calc_ex(bezt, prev, next, flag, true);

		/* For automatic ease in and out. */
		if (BEZT_IS_AUTOH(bezt)) {
			/* Only do this on first or last beztriple. */
			if (ELEM(i, 0, fcu->totvert - 1)) {
				bezt->vec[0][1] = bezt->vec[2][1] = bezt->vec[1][1];
			}
		}
	}

	KER_nurb_handle_smooth_fcurve(fcu->bezt, fcu->totvert, cycle);
}

/** \} */
