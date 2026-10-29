#include "LIB_math_vector.h"
#include "LIB_math_matrix.h"

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