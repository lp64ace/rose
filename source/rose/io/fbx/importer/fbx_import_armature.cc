#include "LIB_listbase.h"
#include "LIB_math_vector.hh"
#include "LIB_math_matrix.hh"
#include "LIB_math_matrix.h"

#include "KER_action.h"
#include "KER_armature.h"
#include "KER_object.h"
#include "KER_lib_id.h"

#include "ED_armature.h"

#include "fbx_import_armature.hh"

#include <iomanip>

namespace rose::io::fbx {

struct ArmatureImportContext {
	const ufbx_scene *fbx;

	Main *main;
	Scene *scene;
	FbxElementMapping *mapping;

	ArmatureImportContext(Main *main, Scene *scene, const ufbx_scene *fbx, FbxElementMapping *mapping) : main(main), scene(scene), fbx(fbx), mapping(mapping) {
	}

	void calc_bone_bind_matrices();
	void find_armatures(const ufbx_node *node);

	Object *create_armature_for_node(const ufbx_node *node);

	void create_armature_bones(const ufbx_node *node, Object *object, const rose::Set<const ufbx_node *> &bone_nodes, EditBone *edit_parent_bone, const ufbx_matrix &parent_mat, const ufbx_matrix &world_to_arm, const float parent_bone_size);
};

Object *ArmatureImportContext::create_armature_for_node(const ufbx_node *node) {
	ROSE_assert_msg(node != nullptr, "fbx: node for armature creation should not be null");

	const char *arm_name = get_fbx_name(node->name, "Armature");
	const char *obj_name = get_fbx_name(node->name, "Armature");

	Armature *armature = KER_armature_add(main, arm_name);
	Object *object = KER_object_add_for_data(this->main, this->scene, OB_ARMATURE, obj_name, &armature->id, false);

	this->mapping->imported_objects.add(object);

	if (!node->is_root) {
		this->mapping->el_to_object.add(&node->element, object);
		node_matrix_to_obj(node, object, this->mapping);

		/* Record world to fbx node matrix for the armature object. */
		ufbx_matrix world_to_arm = ufbx_matrix_invert(&node->node_to_world);
		this->mapping->armature_world_to_arm_node_matrix.add(object, world_to_arm);

		/* Record world to posed root node matrix. */
		if (node->bind_pose && node->bind_pose->is_bind_pose) {
			for (const ufbx_bone_pose &pose : node->bind_pose->bone_poses) {
				if (pose.bone_node == node) {
					world_to_arm = ufbx_matrix_invert(&pose.bone_to_world);
					break;
				}
			}
		}
		this->mapping->armature_world_to_arm_pose_matrix.add(object, world_to_arm);
	}
	else {
		/**
		 * For armatures created at root, make them have the same rotation/scale
		 * as done by ufbx for all regular nodes.
		 */
		ufbx_matrix_to_obj(this->mapping->global_conv_matrix, object);
		ufbx_matrix world_to_arm = ufbx_matrix_invert(&this->mapping->global_conv_matrix);
		this->mapping->armature_world_to_arm_pose_matrix.add(object, world_to_arm);
		this->mapping->armature_world_to_arm_node_matrix.add(object, world_to_arm);
	}

	return object;
}

/* Need to create armature if we are root bone, or any child is a non-root bone. */
ROSE_STATIC bool need_create_armature_for_node(const ufbx_node *node) {
	if (node->bone && node->bone->is_root) {
		return true;
	}
	for (const ufbx_node *fchild : node->children) {
		if (fchild->bone && !fchild->bone->is_root) {
			return true;
		}
	}
	return false;
}

ROSE_STATIC void find_bones(const ufbx_node *node, rose::Set<const ufbx_node *> &r_bones) {
	if (node->bone != nullptr) {
		r_bones.add(node);
	}
	for (const ufbx_node *child : node->children) {
		find_bones(child, r_bones);
	}
}

ROSE_STATIC void find_fake_bones(const ufbx_node *root_node, const rose::Set<const ufbx_node *> &bones, rose::Set<const ufbx_node *> &r_fake_bones) {
	for (const ufbx_node *bone_node : bones) {
		const ufbx_node *node = bone_node->parent;
		while (!ELEM(node, nullptr, root_node)) {
			if (node->bone == nullptr) {
				r_fake_bones.add(node);
			}
			node = node->parent;
		}
	}
}

ROSE_STATIC rose::Set<const ufbx_node *> find_all_bones(const ufbx_node *node) {
	rose::Set<const ufbx_node *> bones;

	find_bones(node, bones);

	rose::Set<const ufbx_node *> fake_bones;
	find_fake_bones(node, bones, fake_bones);
	for (const ufbx_node *bone : fake_bones) {
		bones.add(bone);
	}

	return bones;
}

void ArmatureImportContext::create_armature_bones(const ufbx_node *node, Object *object, const rose::Set<const ufbx_node *> &bone_nodes, EditBone *parent_bone, const ufbx_matrix &parent_mat, const ufbx_matrix &world_to_arm, const float parent_bone_size) {
	ROSE_assert(node != NULL && !node->is_root);

	Armature *armature = static_cast<Armature *>(object->data);

	std::string name;
	if (node->is_geometry_transform_helper) {
		name = get_fbx_name(node->parent->name, "Bone") + std::string("_GeomAdjust");
	}
	else {
		name = get_fbx_name(node->name, "Bone");
	}

	EditBone *bone = ED_armature_ebone_add(armature, &name[0]);
	this->mapping->node_to_name.add(node, bone->name);
	this->mapping->node_is_rose_bone.add(node);
	this->mapping->bone_to_armature.add(node, object);
	bone->parent = parent_bone;

	ufbx_matrix bone_mat = this->mapping->get_node_bind_matrix(node);
	bone_mat = ufbx_matrix_mul(&world_to_arm, &bone_mat);
	bone_mat.cols[0] = ufbx_vec3_normalize(bone_mat.cols[0]);
	bone_mat.cols[1] = ufbx_vec3_normalize(bone_mat.cols[1]);
	bone_mat.cols[2] = ufbx_vec3_normalize(bone_mat.cols[2]);

	float bone_size = 0.0f;
	size_t bone_children_count = 0;

	for (const ufbx_node *fchild : node->children) {
		if (!bone_nodes.contains(fchild)) {
			continue;
		}

		ufbx_vec3 pos = fchild->local_transform.translation;
		if (this->mapping->bone_to_bind_matrix.contains(fchild)) {
			ufbx_matrix local_mat = this->mapping->calc_local_bind_matrix(fchild, world_to_arm);
			pos = local_mat.cols[3];
		}
		bone_size += math::length(float3(pos.x, pos.y, pos.z));
		bone_children_count++;
	}

	if (bone_children_count > 0) {
		bone_size /= bone_children_count;
	}
	else {
		/* This is leaf bone, set length to parent bone length. */
		bone_size = parent_bone_size;
		if (!this->mapping->bone_to_bind_matrix.contains(node)) {
			ufbx_matrix offset_mat = ufbx_transform_to_matrix(&node->local_transform);
			bone_mat = ufbx_matrix_mul(&parent_mat, &offset_mat);
			bone_mat.cols[0] = ufbx_vec3_normalize(bone_mat.cols[0]);
			bone_mat.cols[1] = ufbx_vec3_normalize(bone_mat.cols[1]);
			bone_mat.cols[2] = ufbx_vec3_normalize(bone_mat.cols[2]);
		}
	}

	bone_size = math::max(bone_size, 1e-2f);

	this->mapping->bone_to_length.add(node, bone_size);

	bone->tail[0] = 0.0f;
	bone->tail[1] = bone_size;
	bone->tail[2] = 0.0f;

	float mat[4][4];
	matrix_to_mat4(bone_mat, mat);

	ED_armature_ebone_from_mat4(bone, mat);

	/* Mark bone as connected to parent if head approximately in the same place as parent tail, in
	 * both rest pose and current pose. */
	if (parent_bone != nullptr) {
		float3 self_head_rest(bone->head);
		float3 par_tail_rest(parent_bone->tail);
		const float connect_dist = 1.0e-4f;
		const float connect_dist_sq = connect_dist * connect_dist;
		float dist_sq_rest = math::distance_squared(self_head_rest, par_tail_rest);
		if (dist_sq_rest < connect_dist_sq) {
			/* Bones seem connected in rest pose, now check their current transforms. */
			ufbx_vec3 self_head_cur_u = node->node_to_world.cols[3];
			ufbx_vec3 par_tail;
			par_tail.x = 0;
			par_tail.y = parent_bone_size;
			par_tail.z = 0;
			ufbx_vec3 par_tail_cur_u = ufbx_transform_position(&node->parent->node_to_world, par_tail);
			float3 self_head_cur(self_head_cur_u.x, self_head_cur_u.y, self_head_cur_u.z);
			float3 par_tail_cur(par_tail_cur_u.x, par_tail_cur_u.y, par_tail_cur_u.z);
			float dist_sq_cur = math::distance_squared(self_head_cur, par_tail_cur);

			if (dist_sq_cur < connect_dist_sq) {
				/* Connected in both cases. */
				bone->flag |= BONE_CONNECTED;
			}
		}
	}

	/* Recurse into child bones. */
	for (const ufbx_node *fchild : node->children) {
		if (!bone_nodes.contains(fchild)) {
			continue;
		}

		create_armature_bones(fchild, object, bone_nodes, bone, bone_mat, world_to_arm, bone_size);
	}
}

void ArmatureImportContext::calc_bone_bind_matrices() {
	/**
	 * Figure out bind matrices for bone nodes:
	 * - Get them from "pose" objects in FBX that are marked as "bind pose",
	 * - From all "skin deformer" objects in FBX; these override the ones from "poses".
	 * - For all the bone nodes that do not have a matrix yet, record their world matrix
	 *   as bind matrix.
	 */
	for (const ufbx_pose *fpose : this->fbx->poses) {
		if (!fpose->is_bind_pose) {
			continue;
		}
		for (const ufbx_bone_pose &bone_pose : fpose->bone_poses) {
			const ufbx_matrix &bind_matrix = bone_pose.bone_to_world;
			this->mapping->bone_to_bind_matrix.add_overwrite(bone_pose.bone_node, bind_matrix);
		}
	}

	for (const ufbx_skin_deformer *fskin : this->fbx->skin_deformers) {
		for (const ufbx_skin_cluster *fbone : fskin->clusters) {
			const ufbx_matrix &bind_matrix = fbone->bind_to_world;
			this->mapping->bone_to_bind_matrix.add_overwrite(fbone->bone_node, bind_matrix);
			this->mapping->bone_is_skinned.add(fbone->bone_node);
		}
	}
}

void ArmatureImportContext::find_armatures(const ufbx_node *node) {
	const bool needs_armature = need_create_armature_for_node(node);
	if (needs_armature) {
		Object *object = this->create_armature_for_node(node);
		ufbx_matrix world_to_arm = this->mapping->armature_world_to_arm_pose_matrix.lookup_default(object, ufbx_identity_matrix);

		rose::Set<const ufbx_node *> bone_nodes = find_all_bones(node);

		// Create armature, bones, using edit mode.

		Armature *armature = static_cast<Armature *>(object->data);
		ED_armature_edit_new(armature);
		this->mapping->node_to_name.add(node, object->id.name);
		for (const ufbx_node *fchild : node->children) {
			if (bone_nodes.contains(fchild)) {
				create_armature_bones(fchild, object, bone_nodes, NULL, ufbx_identity_matrix, world_to_arm, 1.0f);
			}
		}

		ED_armature_edit_apply(this->main, armature);
		ED_armature_edit_free(armature);

		// Create pose, and custom properties on the bone pose channels.

		for (const ufbx_node *fbone : bone_nodes) {
			if (!this->mapping->node_is_rose_bone.contains(fbone)) {
				continue; /* Rose bone was not created for it (e.g. root bone in some cases). */
			}
			PoseChannel *pchannel = KER_pose_channel_find_name(object->pose, this->mapping->node_to_name.lookup_default(fbone, "").c_str());
			if (pchannel == nullptr) {
				continue;
			}

			// We should be able to read custom properties here.

			/**
			 * For bones that have rest/bind information, put their current transform into
			 * the current pose.
			 */
			if (this->mapping->bone_to_bind_matrix.contains(fbone)) {
				ufbx_matrix bind_local_mtx = this->mapping->calc_local_bind_matrix(fbone, world_to_arm);
				ufbx_matrix bind_local_mtx_inv = ufbx_matrix_invert(&bind_local_mtx);
				ufbx_transform xform = fbone->local_transform;
				if (fbone->node_depth <= 1) {
					ufbx_matrix matrix = ufbx_matrix_mul(&world_to_arm, &fbone->node_to_world);
					xform = ufbx_matrix_to_transform(&matrix);
				}
				ufbx_matrix pose_mtx = calc_bone_pose_matrix(xform, *fbone, bind_local_mtx_inv);

				float pchan_matrix[4][4];
				matrix_to_mat4(pose_mtx, pchan_matrix);
				KER_pose_channel_apply_mat4(pchannel, pchan_matrix, false);
			}
		}
	}

	/* Recurse into children that have not been turned into bones yet. */
	for (const ufbx_node *fchild : node->children) {
		if (!this->mapping->node_is_rose_bone.contains(fchild)) {
			this->find_armatures(fchild);
		}
	}
}

void import_armatures(Main *main, Scene *scene, const ufbx_scene *fbx, FbxElementMapping *mapping) {
	ArmatureImportContext ctx(main, scene, fbx, mapping);

	ctx.calc_bone_bind_matrices();
	ctx.find_armatures(fbx->root_node);
}

}  // namespace rose::io::fbx
