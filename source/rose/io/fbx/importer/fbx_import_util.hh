#ifndef IO_FBX_IMPORT_UTIL_HH
#define IO_FBX_IMPORT_UTIL_HH

#include "KER_object.h"

#include "LIB_set.hh"
#include "LIB_map.hh"
#include "LIB_utildefines.h"

#include <ufbx.h>

namespace rose::io::fbx {

struct FbxElementMapping {
	rose::Set<Object *> imported_objects;
	rose::Map<const ufbx_element *, Object *> el_to_object;
	rose::Map<const ufbx_node *, Object *> bone_to_armature;

	/**
	 * For the armatures we create, for different use cases we need transform
	 * from world space to the root bone, either in posed transform or in
	 * node transform.
	 */
	rose::Map<const Object *, ufbx_matrix> armature_world_to_arm_pose_matrix;
	rose::Map<const Object *, ufbx_matrix> armature_world_to_arm_node_matrix;

	/**
	 * Which FBX bone nodes got turned into actual armature bones (not all of them
	 * always are; in some cases root bone is the armature object itself).
	 */
	rose::Set<const ufbx_node *> node_is_rose_bone;
	rose::Map<const ufbx_node *, std::string> node_to_name;
	/**
	 * Bone node to "bind matrix", i.e. matrix that transforms from bone (in skin bind pose) local
	 * space to world space. This records bone pose or skin cluster bind matrix (skin cluster taking
	 * precedence if it exists).
	 */
	rose::Map<const ufbx_node *, ufbx_matrix> bone_to_bind_matrix;
	rose::Map<const ufbx_node *, ufbx_real> bone_to_length;
	rose::Set<const ufbx_node *> bone_is_skinned;
	ufbx_matrix global_conv_matrix;

	ufbx_matrix get_node_bind_matrix(const ufbx_node *node) const {
		return this->bone_to_bind_matrix.lookup_default(node, node->geometry_to_world);
	}

	ufbx_matrix calc_local_bind_matrix(const ufbx_node *bone_node, const ufbx_matrix &world_to_arm) const {
		ufbx_matrix res = this->get_node_bind_matrix(bone_node);
		ufbx_matrix parent_inv_mtx;
		if (bone_node->parent != nullptr && !bone_node->parent->is_root) {
			ufbx_matrix parent_mtx = this->get_node_bind_matrix(bone_node->parent);
			parent_inv_mtx = ufbx_matrix_invert(&parent_mtx);
		}
		else {
			parent_inv_mtx = world_to_arm;
		}
		res = ufbx_matrix_mul(&parent_inv_mtx, &res);
		return res;
	}
};

void matrix_to_mat4(const ufbx_matrix &src, float dst[4][4]);
void ufbx_matrix_to_obj(const ufbx_matrix &src, Object *object);
void node_matrix_to_obj(const ufbx_node *src, Object *object, const FbxElementMapping *mapping);

/** Returns the name of the node or falls back to the default provided name. */
const char *get_fbx_name(const ufbx_string &name, const char *def);

ufbx_matrix calc_bone_pose_matrix(const ufbx_transform &local_xform, const ufbx_node &node, const ufbx_matrix &local_bind_inv_matrix);

}  // namespace rose::io::fbx

#endif	// IO_FBX_IMPORT_UTIL_HH
