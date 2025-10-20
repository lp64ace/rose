#include "fbx_import_util.hh"

#include "LIB_math_matrix.h"
#include "LIB_math_matrix.hh"

namespace rose::io::fbx {

const char *get_fbx_name(const ufbx_string &name, const char *def) {
	return name.length > 0 ? name.data : def;
}

void matrix_to_mat4(const ufbx_matrix &src, float dst[4][4]) {
	dst[0][0] = src.m00;
	dst[1][0] = src.m01;
	dst[2][0] = src.m02;
	dst[3][0] = src.m03;
	dst[0][1] = src.m10;
	dst[1][1] = src.m11;
	dst[2][1] = src.m12;
	dst[3][1] = src.m13;
	dst[0][2] = src.m20;
	dst[1][2] = src.m21;
	dst[2][2] = src.m22;
	dst[3][2] = src.m23;
	dst[0][3] = 0.0f;
	dst[1][3] = 0.0f;
	dst[2][3] = 0.0f;
	dst[3][3] = 1.0f;
}

void ufbx_matrix_to_obj(const ufbx_matrix &mtx, Object *obj) {
	float obmat[4][4];
	matrix_to_mat4(mtx, obmat);
	KER_object_apply_mat4(obj, obmat, true, false);
	KER_object_to_mat4(obj, obj->runtime.object_to_world);
	invert_m4_m4(obj->runtime.world_to_object, obj->runtime.object_to_world);
}

void node_matrix_to_obj(const ufbx_node *node, Object *obj, const FbxElementMapping *mapping) {
	ufbx_matrix mtx = ufbx_matrix_mul(node->node_depth < 2 ? &node->node_to_world : &node->node_to_parent, &node->geometry_to_node);

	/* Handle case of an object parented to a bone: need to set
	 * bone as parent, and make transform be at the end of the bone. */
	const ufbx_node *parbone = node->parent;
	if (obj->parent == nullptr && parbone && mapping->node_is_rose_bone.contains(parbone)) {
		Object *arm = mapping->bone_to_armature.lookup_default(parbone, nullptr);
		if (arm != nullptr) {
			ufbx_matrix offset_mtx = ufbx_identity_matrix;
			offset_mtx.cols[3].y = -mapping->bone_to_length.lookup_default(parbone, 0.0);
			if (mapping->node_is_rose_bone.contains(node)) {
				/* The node itself is a "fake bone", in which case parent it to the matching
				 * fake bone, and matrix is just what puts transform at the bone tail. */
				parbone = node;
				mtx = offset_mtx;
			}
			else {
				mtx = ufbx_matrix_mul(&offset_mtx, &mtx);
			}

			obj->parent = arm;
			obj->partype = PARBONE;
			LIB_strcpy(obj->parsubstr, ARRAY_SIZE(obj->parsubstr), mapping->node_to_name.lookup_default(parbone, "").c_str());
		}
	}

	ufbx_matrix_to_obj(mtx, obj);
}

ufbx_matrix calc_bone_pose_matrix(const ufbx_transform &local_xform, const ufbx_node &node, const ufbx_matrix &local_bind_inv_matrix) {
	ufbx_transform xform = local_xform;

	/* For bones that have "ignore parent scale" on them, ufbx helpfully applies global scale to
	 * the evaluated transform. However we really need to get local transform without global
	 * scale, so undo that. */
	if (node.adjust_post_scale != 1.0) {
		xform.scale.x /= node.adjust_post_scale;
		xform.scale.y /= node.adjust_post_scale;
		xform.scale.z /= node.adjust_post_scale;
	}

	/* Transformed to the bind transform in joint-local space. */
	ufbx_matrix matrix = ufbx_transform_to_matrix(&xform);
	matrix = ufbx_matrix_mul(&local_bind_inv_matrix, &matrix);
	return matrix;
}

}  // namespace rose::io::fbx
