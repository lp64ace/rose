#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_view3d_types.h"

#include "ED_view3d.h"

#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

float ED_view3d_calc_zfac_ex(const RegionView3D *rv3d, const float co[3], bool *r_flip) {
	float zfac = mul_project_m4_v3_zfac(rv3d->winmat, co);

	if (r_flip) {
		*r_flip = (zfac < 0.0f);
	}

	/* if x,y,z is exactly the viewport offset, zfac is 0 and we don't want that
	* (accounting for near zero values) */
	if (zfac < 1.e-6f && zfac > -1.e-6f) {
		zfac = 1.0f;
	}

	/* Negative zfac means x, y, z was behind the camera (in perspective).
	* This gives flipped directions, so revert back to ok default case. */
	if (zfac < 0.0f) {
		zfac = -zfac;
	}

	return zfac;
}

float ED_view3d_calc_zfac(const RegionView3D *rv3d, const float co[3]) {
	return ED_view3d_calc_zfac_ex(rv3d, co, NULL);
}

void ED_view3d_win_to_delta(const ARegion *region, const float xy_delta[2], const float zfac, float r_out[3]) {
	RegionView3D *rv3d = region->regiondata;
	float dx, dy;

	dx = 2.0f * xy_delta[0] * zfac / region->sizex;
	dy = 2.0f * xy_delta[1] * zfac / region->sizey;

	r_out[0] = (rv3d->wininv[0][0] * dx + rv3d->wininv[1][0] * dy);
	r_out[1] = (rv3d->wininv[0][1] * dx + rv3d->wininv[1][1] * dy);
	r_out[2] = (rv3d->wininv[0][2] * dx + rv3d->wininv[1][2] * dy);
}

ROSE_INLINE eV3DProjStatus ed_view3d_project_internal(const ARegion *region, const float perspmat[4][4], const bool is_local, const float co[3], float r_co[2], const eV3DProjTest flag) {
	float vec4[4];

	/* Check for bad flags */
	ROSE_assert((flag & V3D_PROJ_TEST_ALL) == flag);

	copy_v3_v3(vec4, co);
	vec4[3] = 1.0;
	mul_m4_v4(perspmat, vec4);
	const float w = fabsf(vec4[3]);

	if ((flag & V3D_PROJ_TEST_CLIP_ZERO) && (w <= (float)1e-3f)) {
		return V3D_PROJ_RET_CLIP_ZERO;
	}

	if ((flag & V3D_PROJ_TEST_CLIP_NEAR) && (vec4[2] <= -w)) {
		return V3D_PROJ_RET_CLIP_NEAR;
	}

	if ((flag & V3D_PROJ_TEST_CLIP_FAR) && (vec4[2] >= w)) {
		return V3D_PROJ_RET_CLIP_FAR;
	}

	const float scalar = (w != 0.0f) ? (1.0f / w) : 0.0f;
	const float fx = ((float)region->sizex / 2.0f) * (1.0f + (vec4[0] * scalar));
	const float fy = ((float)region->sizey / 2.0f) * (1.0f + (vec4[1] * scalar));

	if ((flag & V3D_PROJ_TEST_CLIP_WIN) && (fx <= 0.0f || fy <= 0.0f || fx >= (float)region->sizex || fy >= (float)region->sizey)) {
		return V3D_PROJ_RET_CLIP_WIN;
	}

	r_co[0] = fx;
	r_co[1] = fy;

	return V3D_PROJ_RET_OK;
}

eV3DProjStatus ED_view3d_project_int_ex(const struct ARegion *region, float perspmat[4][4], const bool is_local, const float co[3], int r_co[2], eV3DProjTest flag) {
	float tvec[2];
	eV3DProjStatus ret = ed_view3d_project_internal(region, perspmat, is_local, co, tvec, flag);
	if (ret == V3D_PROJ_RET_OK) {
		if ((tvec[0] > INT_MIN && tvec[0] < INT_MAX) &&
			(tvec[1] > INT_MIN && tvec[1] < INT_MAX)) {
			r_co[0] = (int)floorf(tvec[0]);
			r_co[1] = (int)floorf(tvec[1]);
		}
		else {
			ret = V3D_PROJ_RET_OVERFLOW;
		}
	}
	return ret;
}

eV3DProjStatus ED_view3d_project_int_global(const struct ARegion *region, const float co[3], int r_co[2], eV3DProjTest flag) {
	RegionView3D *rv3d = region->regiondata;
	return ED_view3d_project_int_ex(region, rv3d->winmat, false, co, r_co, flag);
}

eV3DProjStatus ED_view3d_project_float_ex(const ARegion *region, float perspmat[4][4], const bool is_local, const float co[3], float r_co[2], const eV3DProjTest flag) {
	float tvec[2];
	eV3DProjStatus ret = ed_view3d_project_internal(region, perspmat, is_local, co, tvec, flag);
	if (ret == V3D_PROJ_RET_OK) {
		if (isfinite(tvec[0]) && isfinite(tvec[1])) {
			copy_v2_v2(r_co, tvec);
		}
		else {
			ret = V3D_PROJ_RET_OVERFLOW;
		}
	}
	return ret;
}

eV3DProjStatus ED_view3d_project_float_global(const ARegion *region, const float co[3], float r_co[2], eV3DProjTest flag) {
	RegionView3D *rv3d = region->regiondata;
	return ED_view3d_project_float_ex(region, rv3d->winmat, false, co, r_co, flag);
}
