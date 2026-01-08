#include "KER_action.h"
#include "KER_armature.h"
#include "KER_deform.h"
#include "KER_lib_id.h"
#include "KER_mesh.h"
#include "KER_mesh.hh"
#include "KER_modifier.h"
#include "KER_object.h"
#include "KER_object_deform.h"

#include "LIB_task.hh"
#include "LIB_listbase.h"
#include "LIB_math_matrix.hh"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.hh"
#include "LIB_math_vector.h"
#include "LIB_span.hh"
#include "LIB_vector_set.hh"
#include "LIB_utildefines.h"

#include "fbx_import_mesh.hh"

#include <iomanip>

namespace rose::io::fbx {

ROSE_INLINE void import_vertex_positions(const ufbx_mesh *fmesh, Mesh *mesh) {
	rose::MutableSpan<float3> positions = KER_mesh_vert_positions_for_write_span(mesh);

	ROSE_assert(positions.size() == fmesh->vertex_position.values.count);
	for (size_t index = 0; index < fmesh->vertex_position.values.count; index++) {
		ufbx_vec3 val = fmesh->vertex_position.values[index];
		positions[index] = float3(val.x, val.y, val.z);
	}
}

ROSE_INLINE void import_face_smoothing(const ufbx_mesh *fmesh, Mesh *mesh) {
	if (fmesh->face_smoothing.count > 0 && fmesh->face_smoothing.count == fmesh->num_faces) {
		rose::MutableSpan<bool> smooth = rose::MutableSpan<bool>{
			static_cast<bool *>(CustomData_get_layer_named_for_write(&mesh->vdata, CD_PROP_BOOL, "sharp_face", mesh->totpoly)),
			(size_t)mesh->totpoly
		};
		for (size_t i = 0; i < fmesh->face_smoothing.count; i++) {
			smooth[i] = !fmesh->face_smoothing[i];
		}
	}
}

ROSE_INLINE void import_faces(const ufbx_mesh *fmesh, Mesh *mesh) {
	rose::MutableSpan<int> face_offsets = KER_mesh_poly_offsets_for_write_span(mesh);
	rose::MutableSpan<int> corner_verts = KER_mesh_corner_verts_for_write_span(mesh);

	ROSE_assert((face_offsets.size() == fmesh->num_faces + 1) || (face_offsets.is_empty() && fmesh->num_faces == 0));
	for (size_t findex = 0; findex < fmesh->num_faces; findex++) {
		const ufbx_face &fface = fmesh->faces[findex];
		face_offsets[findex] = fface.index_begin;
		for (size_t lindex = 0; lindex < fface.num_indices; lindex++) {
			size_t cindex = fface.index_begin + lindex;
			size_t vertex = fmesh->vertex_indices[cindex];
			corner_verts[cindex] = vertex;
		}
	}
}

ROSE_INLINE void import_edges(const ufbx_mesh *fmesh, Mesh *mesh) {
	rose::MutableSpan<int2> edges = KER_mesh_edges_for_write_span(mesh);

	ROSE_assert(edges.size() == fmesh->num_edges);
	for (size_t index = 0; index < fmesh->num_edges; index++) {
		const ufbx_edge &fedge = fmesh->edges[index];
		const size_t va = fmesh->vertex_indices[fedge.a];
		const size_t vb = fmesh->vertex_indices[fedge.b];
		edges[index] = int2(va, vb);
	}
}

ROSE_INLINE rose::VectorSet<std::string> get_skin_bone_name_set(const FbxElementMapping *mapping, const ufbx_mesh *fmesh) {
	rose::VectorSet<std::string> name_set;
	for (const ufbx_skin_deformer *skin : fmesh->skin_deformers) {
		for (const ufbx_skin_cluster *cluster : skin->clusters) {
			if (cluster->num_weights == 0) {
				continue;
			}

			std::string bone_name = mapping->node_to_name.lookup_default(cluster->bone_node, "");
			name_set.add(bone_name);
		}
	}
	return name_set;
}

static bool is_skin_deformer_usable(const ufbx_mesh *mesh, const ufbx_skin_deformer *skin) {
	return mesh != nullptr && skin != nullptr && skin->clusters.count > 0 && mesh->num_vertices > 0 && skin->vertices.count == mesh->num_vertices;
}

ROSE_INLINE void import_skin_vertex_groups(const FbxElementMapping *mapping, const ufbx_mesh *fmesh, Mesh *mesh) {
	if (fmesh->skin_deformers.count == 0) {
		return;
	}

	rose::VectorSet<std::string> bone_set = get_skin_bone_name_set(mapping, fmesh);
	if (bone_set.is_empty()) {
		return;
	}

	rose::MutableSpan<MDeformVert> dverts = KER_mesh_deform_verts_for_write_span(mesh);

	for (const ufbx_skin_deformer *skin : fmesh->skin_deformers) {
		if (!is_skin_deformer_usable(fmesh, skin)) {
			continue;
		}

		for (const ufbx_skin_cluster *cluster : skin->clusters) {
			if (cluster->num_weights == 0) {
				continue;
			}

			std::string bone_name = mapping->node_to_name.lookup_default(cluster->bone_node, "");
			const int group_index = bone_set.index_of_try(bone_name);
			if (group_index < 0) {
				continue;
			}

			for (size_t index = 0; index < cluster->num_weights; index++) {
				const size_t vertex = cluster->vertices[index];
				if (vertex < dverts.size()) {
					MDeformWeight *dw = KER_defvert_ensure_index(&dverts[vertex], group_index);
					dw->weight = cluster->weights[index];
				}
			}
		}
	}
}

void import_meshes(Main *main, Scene *scene, const ufbx_scene *fbx, FbxElementMapping *mapping) {
	rose::Vector<Mesh *> meshes(fbx->meshes.count);
	rose::threading::parallel_for_each(IndexRange(fbx->meshes.count), [&](const size_t index) {
		const ufbx_mesh *fmesh = fbx->meshes.data[index];
		if (fmesh->instances.count == 0) {
			meshes[index] = nullptr;
			return;
		}

		Mesh *mesh = static_cast<Mesh *>(KER_object_obdata_add_from_type(main, OB_MESH, get_fbx_name(fmesh->name, "Mesh")));

		mesh->totvert = fmesh->num_vertices;
		mesh->totedge = fmesh->num_edges;
		mesh->totpoly = fmesh->num_faces;
		mesh->totloop = fmesh->num_indices;

		KER_mesh_ensure_required_data_layers(mesh);

		KER_mesh_poly_offsets_ensure_alloc(mesh);

		import_vertex_positions(fmesh, mesh);
		import_faces(fmesh, mesh);
		import_face_smoothing(fmesh, mesh);
		import_edges(fmesh, mesh);
		import_skin_vertex_groups(mapping, fmesh, mesh);

		meshes[index] = mesh;
	});

	for (size_t index : meshes.index_range()) {
		Mesh *mesh = meshes[index];
		if (mesh == nullptr) {
			continue;
		}

		const ufbx_mesh *fmesh = fbx->meshes[index];

		for (const ufbx_node *node : fmesh->instances) {
			std::string name;
			if (node->is_geometry_transform_helper) {
				/* Name geometry transform adjustment helpers with parent name and _GeomAdjust suffix. */
				name = get_fbx_name(node->parent->name, "Object") + std::string("_GeomAdjust");
			}
			else {
				name = get_fbx_name(node->name, "Object");
			}

			Object *object = KER_object_add_for_data(main, scene, OB_MESH, &name[0], &mesh->id, false);

			bool matrix_already_set = false;

			if (fmesh->skin_deformers.count > 0) {
				/* Add vertex groups to the object. */
				rose::VectorSet<std::string> bone_set = get_skin_bone_name_set(mapping, fmesh);
				for (const std::string &name : bone_set) {
					KER_object_defgroup_add_name(object, name.c_str());
				}

				for (const ufbx_skin_deformer *skin : fmesh->skin_deformers) {
					if (!is_skin_deformer_usable(fmesh, skin)) {
						continue;
					}

					Object *armature = NULL;
					for (const ufbx_skin_cluster *cluster : skin->clusters) {
						if (cluster->num_weights == 0) {
							continue;
						}

						armature = mapping->bone_to_armature.lookup_default(cluster->bone_node, NULL);
						if (armature != NULL) {
							break;
						}
					}
					
					if (armature != NULL) {
						ModifierData *md = KER_modifier_new(MODIFIER_TYPE_ARMATURE);

						/**
						 * Assign the modifier to the object that is to be deformed.
						 */
						LIB_addtail(&object->modifiers, md);

						ArmatureModifierData *ad = reinterpret_cast<ArmatureModifierData *>(md);
						ad->modifier.flag |= MODIFIER_DEVICE_ONLY;
						/**
						 * Assign the armature object to the modifier.
						 */
						ad->object = armature;

						if (!matrix_already_set) {
							matrix_already_set = true;
							object->parent = armature;

							/* We are setting mesh parent to the armature, so set the matrix that is
							 * armature-local. Note that the matrix needs to be relative to the FBX
							 * node matrix (not the root bone pose matrix). */
							ufbx_matrix world_to_arm = mapping->armature_world_to_arm_node_matrix.lookup_default(armature, ufbx_identity_matrix);
							ufbx_matrix world_to_arm_pose = mapping->armature_world_to_arm_pose_matrix.lookup_default(armature, ufbx_identity_matrix);

							ufbx_matrix mtx = ufbx_matrix_mul(&world_to_arm, &node->geometry_to_world);
							ufbx_matrix_to_obj(mtx, object);

							/* Setup parent inverse matrix of the mesh, to account for the mesh possibly being in
							 * different bind pose than what the node is at. */
							ufbx_matrix mtx_inv = ufbx_matrix_invert(&mtx);
							ufbx_matrix mtx_world = mapping->get_node_bind_matrix(node);
							ufbx_matrix mtx_parent_inverse = ufbx_matrix_mul(&mtx_world, &mtx_inv);
							mtx_parent_inverse = ufbx_matrix_mul(&world_to_arm_pose, &mtx_parent_inverse);
							matrix_to_mat4(mtx_parent_inverse, object->parentinv);
						}
					}
				}
			}

			if (!matrix_already_set) {
				node_matrix_to_obj(node, object, mapping);
			}
			mapping->el_to_object.add(&node->element, object);
			mapping->imported_objects.add(object);
		}
	}
}

}  // namespace rose::io::fbx
