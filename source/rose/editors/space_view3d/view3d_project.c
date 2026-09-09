#include "DNA_screen_types.h"
#include "DNA_space_types.h"
#include "DNA_view3d_types.h"

#include "ED_view3d.h"

#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_utildefines.h"

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
