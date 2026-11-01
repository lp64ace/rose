#include "KER_context.h"

#include "KER_layer.h"
#include "KER_object.h"
#include "KER_scene.h"

#include "DNA_view3d_types.h"

#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"

#include "transform.h"
#include "transform_orientations.h"

bool transform_orientations_create_from_axis(float mat[3][3], const float x[3], const float y[3], const float z[3]) {
	bool is_zero[3] = {true, true, true};
	zero_m3(mat);
	if (x) {
		is_zero[0] = normalize_v3_v3(mat[0], x) == 0.0f;
	}
	if (y) {
		is_zero[1] = normalize_v3_v3(mat[1], y) == 0.0f;
	}
	if (z) {
		is_zero[2] = normalize_v3_v3(mat[2], z) == 0.0f;
	}

	int zero_axis = is_zero[0] + is_zero[1] + is_zero[2];
	if (zero_axis == 0) {
		return true;
	}

	if (zero_axis == 1) {
		int axis = is_zero[0] ? 0 : is_zero[1] ? 1 : 2;
		cross_v3_v3v3(mat[axis], mat[(axis + 1) % 3], mat[(axis + 2) % 3]);
		if (normalize_v3(mat[axis]) != 0.0f) {
			return true;
		}
	}
	else if (zero_axis == 2) {
		int axis, a, b;
		axis = !is_zero[0] ? 0 : !is_zero[1] ? 1 : 2;
		a = (axis + 1) % 3;
		b = (axis + 2) % 3;

		mat[a][a] = 1.0f;
		mat[b][b] = 1.0f;
		project_plane_v3_v3v3(mat[a], mat[a], mat[axis]);
		project_plane_v3_v3v3(mat[b], mat[b], mat[axis]);
		if ((normalize_v3(mat[a]) != 0.0f) && (normalize_v3(mat[b]) != 0.0f)) {
			return true;
		}
	}

	unit_m3(mat);
	return false;
}

int ED_transform_calc_orientation_from_type_ex(const Scene *scene, ViewLayer *view_layer, const View3D *v3d, const RegionView3D *rv3d, Object *object, const short index, const int pivot, float r_mat[3][3]) {
	switch (index) {
		case V3D_ORIENT_GLOBAL: {
			unit_m3(r_mat);
		} break;
		case V3D_ORIENT_LOCAL: {
			if (object) {
				transform_orientations_create_from_axis(r_mat, UNPACK3(object->obmat));
			}
		} break;
		case V3D_ORIENT_VIEW: {
			if (rv3d != NULL) {
				copy_m3_m4(r_mat, rv3d->viewinv);
				normalize_m3(r_mat);
			}
			else {
				unit_m3(r_mat);
			}
		} break;
	}

	return index;
}

void transform_orientations_default_modal_set(TransInfo *t, int type) {
	/* Currently only these types are supported. */
	ROSE_assert(ELEM(type, V3D_ORIENT_GLOBAL, V3D_ORIENT_VIEW));

	if (!(t->flag & T_MODAL)) {
		return;
	}

	if (t->orient.type == type) {
		return;
	}

	View3D *v3d = NULL;
	RegionView3D *rv3d = NULL;

	if ((type == V3D_ORIENT_VIEW) && (t->spacetype == SPACE_VIEW3D) && t->region && (t->region->regiontype == RGN_TYPE_WINDOW)) {
		v3d = t->view;
		rv3d = t->region->regiondata;
	}

	t->orient.type = ED_transform_calc_orientation_from_type_ex(t->scene, t->view_layer, v3d, rv3d, NULL, type, V3D_AROUND_CENTER_BOUNDS, t->orient.matrix);
}

void transform_orientations_current_set(TransInfo *t) {
	copy_m3_m3(t->spacemat, t->orient.matrix);
	invert_m3_m3_safe_ortho(t->spaceinv, t->spacemat);
}
