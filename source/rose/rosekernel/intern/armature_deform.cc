#include "KER_action.h"
#include "KER_armature.h"
#include "KER_mesh.hh"
#include "KER_object.h"
#include "KER_object_deform.h"

#include "LIB_array.hh"
#include "LIB_listbase.h"
#include "LIB_math_matrix.hh"
#include "LIB_math_quaternion.hh"
#include "LIB_math_vector.hh"
#include "LIB_span.hh"
#include "LIB_task.hh"

#include <iostream>

namespace rose::kernel {

/**
 * Utility class for accumulating linear bone deformation.
 * If full_deform is true the deformation matrix is also computed.
 */
template<bool full_deform> struct BoneDeformLinearMixer {
	float3 position_delta = float3(0.0f);
	float3x3 deform = float3x3::zero();

	void accumulate(const PoseChannel &pchan, const float3 &co, const float weight) {
		const float4x4 &pose_mat = float4x4(pchan.chan_mat);

		position_delta += weight * (math::transform_point(pose_mat, co) - co);
		if constexpr (full_deform) {
			deform += weight * pose_mat.view<3, 3>();
		}
	}

	void finalize(const float3 & /*co*/, float total, float armature_weight, float3 &r_delta_co, float3x3 &r_deform_mat) {
		const float scale_factor = armature_weight / total;
		r_delta_co = position_delta * scale_factor;
		r_deform_mat = deform * scale_factor;
	};
};

struct ArmatureDeformParams {
	rose::MutableSpan<float3> vert_coords;
	rose::MutableSpan<float3x3> vert_deform_mats;

	bool use_dverts;

	const ListBase *pose_channels;

	rose::Array<PoseChannel *> pose_channel_by_vertex_group;

	float4x4 target_to_armature;
	float4x4 armature_to_target;
};

ROSE_INLINE ArmatureDeformParams get_armature_deform_params(const Object *obarmature, const Object *obtarget, const ListBase *defbase, const rose::MutableSpan<float3> vert_coords, const rose::MutableSpan<float3x3> vert_deform_mats) {
	ArmatureDeformParams deform_params;

	deform_params.vert_coords = vert_coords;
	deform_params.vert_deform_mats = vert_deform_mats;
	deform_params.pose_channels = &obarmature->pose->channelbase;

	deform_params.use_dverts = true;

	if (deform_params.use_dverts) {
		const size_t defbase_length = LIB_listbase_count(defbase);

		deform_params.pose_channel_by_vertex_group.reinitialize(defbase_length);

		size_t index;
		LISTBASE_FOREACH_INDEX(DeformGroup *, dg, defbase, index) {
			PoseChannel *pchannel = KER_pose_channel_find_name(obarmature->pose, dg->name);

			deform_params.pose_channel_by_vertex_group[index] = (pchannel && !(pchannel->bone->flag & BONE_NO_DEFORM)) ? pchannel : NULL;
		}
	}

	deform_params.armature_to_target = float4x4(KER_object_world_to_object(obtarget)) * float4x4(KER_object_object_to_world(obarmature));
	deform_params.target_to_armature = float4x4(KER_object_world_to_object(obarmature)) * float4x4(KER_object_object_to_world(obtarget));

	return deform_params;
}

template<typename MixerT> ROSE_INLINE float pchannel_bone_deform(const PoseChannel *pchannel, const float weight, const float3 &co, MixerT &mixer) {
	const Bone *bone = pchannel->bone;

	if (!weight) {
		return 0.0f;
	}

	mixer.accumulate(*pchannel, co, weight);

	return weight;
}

/**
 * armature_to_target [(1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1)]
 * target_to_armature [(1, -0, 0, -0), (-0, 1, -0, 0), (0, -0, 1, -0), (-0, 0, -0, 1)]
 * Deforming vertex 4768
 *  << in (7.78784, -8.81405, 71.2185)
 *         Vertex (7.78784, -8.81405, 71.2185) deformed 0.188235 * skinned_c_pelvis_bn
 * Deforming vertex 4767
 *  << in (5.8639, -10.8033, 67.2812)
 *         Vertex (7.78784, -8.81405, 71.2185) deformed 0.196078 * skinned_c_lumbar_bn
 *         Vertex (5.8639, -10.8033, 67.2812) deformed 0.301961 * skinned_c_pelvis_bn
 *         Vertex (7.78784, -8.81405, 71.2185) deformed 0.160784 * skinned_c_thoracic1_bn
 *         Vertex (5.8639, -10.8033, 67.2812) deformed 0.0431373 * skinned_c_thoracic2_bn
 *         Vertex (7.78784, -8.81405, 71.2185) deformed 0.454902 * skinned_c_thoracic2_bn
 *         Vertex (5.8639, -10.8033, 67.2812) deformed  >> out (6.63813, -7.18345, 69.1281)0.317647 * skinned_c_sacrum_bn
 * Deforming vertex 4769
 *  << in (9.50692, -7.28301, 75.8027)
 *         Vertex (5.8639, -10.8033, 67.2812) deformed 0.337255 * skinned_r_femur_bn
 *         Vertex (9.50692, -7.28301, 75.8027) deformed 0.145098 * skinned_c_thoracic1_bn
 *  >> out (3.0163, -8.78124, 65.8423)     Vertex (9.50692, -7.28301, 75.8027) deformed 0.694118 * skinned_c_thoracic2_bn
 *         Vertex (9.50692, -7.28301, 75.8027) deformed 0.160784 * skinned_c_thoracic3_bn
 *  >> out (9.22381, -5.4776, 73.3764)
 */

template<typename MixerT> ROSE_INLINE void armature_vert_task_with_mixer(const ArmatureDeformParams &params, const size_t index, const MDeformVert *dvert, MixerT &mixer) {
	const bool full_deform = !params.vert_deform_mats.is_empty();

	float3 co = params.vert_coords[index];

	/* Transform to armature space. */
	co = rose::math::transform_point(params.target_to_armature, co);

	float contrib = 0.0f;
	bool deformed = false;
	/* Apply vertex group deformation if enabled. */
	if (params.use_dverts && dvert) {
		const rose::IndexRange def_nr_range = params.pose_channel_by_vertex_group.index_range();
		const rose::Span<MDeformWeight> dweights(dvert->dw, dvert->totweight);

		for (const auto &dw : dweights) {
			const PoseChannel *pchannel = def_nr_range.contains(dw.def_nr) ? params.pose_channel_by_vertex_group[dw.def_nr] : NULL;

			if (pchannel == NULL) {
				continue;
			}

			float weight = dw.weight;

			contrib += pchannel_bone_deform(pchannel, weight, co, mixer);
			deformed = true;
		}
	}

	/* TODO Actually should be EPSILON? Weight values and contrib can be like 10e-39 small. */
	constexpr float contrib_threshold = 1e-7f;
	if (contrib > contrib_threshold) {
		float3 delta_co;
		float3x3 local_deform_mat;
		mixer.finalize(co, contrib, 1.0f, delta_co, local_deform_mat);

		co += delta_co;

		if (full_deform) {
			float3x3 &deform_mat = params.vert_deform_mats[index];
			const float3x3 armature_to_target = params.armature_to_target.view<3, 3>();
			const float3x3 target_to_armature = params.target_to_armature.view<3, 3>();
			deform_mat = armature_to_target * local_deform_mat * target_to_armature * deform_mat;
		}
	}

	/* Transform back to target object space. */
	co = math::transform_point(params.armature_to_target, co);

	params.vert_coords[index] = co;
}

ROSE_INLINE void armature_vert_task_with_dvert(const ArmatureDeformParams &params, const size_t index, const MDeformVert *dvert) {
	const bool full_deform = !params.vert_deform_mats.is_empty();
	if (full_deform) {
		rose::kernel::BoneDeformLinearMixer<true> mixer;
		armature_vert_task_with_mixer(params, index, dvert, mixer);
	}
	else {
		rose::kernel::BoneDeformLinearMixer<false> mixer;
		armature_vert_task_with_mixer(params, index, dvert, mixer);
	}
}

ROSE_INLINE void armature_deform_coords(const Object *obarmature, const Object *obtarget, const ListBase *defbase, const rose::MutableSpan<float3> vert_coords, const rose::MutableSpan<float3x3> vert_deform_mats, rose::Span<MDeformVert> dverts, const Mesh *metarget) {
	ArmatureDeformParams params = get_armature_deform_params(obarmature, obtarget, defbase, vert_coords, vert_deform_mats);

	rose::threading::parallel_for(vert_coords.index_range(), 32, [&](const IndexRange range) {
		for (const size_t index : range) {
			const MDeformVert *dvert = NULL;
			if (params.use_dverts) {
				if (metarget) {
					ROSE_assert(index < metarget->totvert);
					dvert = &(dverts[index]);
				}
				else if (index < dverts.size()) {
					dvert = &(dverts[index]);
				}
			}

			armature_vert_task_with_dvert(params, index, dvert);
		}
	});
}

}  // namespace rose::kernel

using namespace rose::kernel;

/* -------------------------------------------------------------------- */
/** \name Armature Mesh Deformation
 * \{ */

void KER_armature_deform_coords_with_mesh(Object *armature, Object *obtarget, float (*positions)[3], size_t length, Mesh *metarget) {
	const ID *idtarget = (ID *)(&obtarget->id);
	const ListBase *defbase = NULL;
	if (metarget) {
		defbase = KER_id_defgroup_list_get(&metarget->id);
	}
	else {
		defbase = KER_id_defgroup_list_get(&obtarget->id);
	}

	rose::Span<MDeformVert> dverts;

	switch (obtarget->type) {
		case OB_MESH: {
			dverts = KER_mesh_deform_verts_span(metarget);
		} break;
	}

	rose::MutableSpan<float3> vert_coords = rose::MutableSpan<float3>(reinterpret_cast<float3 *>(positions), length);

	armature_deform_coords(armature, obtarget, defbase, vert_coords, rose::MutableSpan<float3x3>(), dverts, metarget);
}

/** \} */