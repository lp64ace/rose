#include "MEM_guardedalloc.h"

#include "DNA_meshdata_types.h"

#include "DRW_cache.h"

#include "KER_action.h"
#include "KER_armature.h"
#include "KER_mesh.hh"
#include "KER_object.h"
#include "KER_object_deform.h"

#include "GPU_index_buffer.h"
#include "GPU_uniform_buffer.h"
#include "GPU_vertex_buffer.h"

#include "LIB_assert.h"
#include "LIB_array_utils.hh"
#include "LIB_math_matrix_types.hh"
#include "LIB_math_vector_types.hh"
#include "LIB_listbase.h"
#include "LIB_span.hh"
#include "LIB_utildefines.h"

#include "extract_mesh.h"

struct ArmatureDeviceDeformParams {
	const ListBase *pose_channels;

	rose::Array<PoseChannel *> pose_channel_by_vertex_group;

	float4x4 target_to_armature;
	float4x4 armature_to_target;
};

ROSE_INLINE ArmatureDeviceDeformParams get_armature_device_deform_params(const Object *obarmature, const Object *obtarget, const ListBase *defbase) {
	ArmatureDeviceDeformParams deform_params;

	deform_params.pose_channels = &obarmature->pose->channelbase;

	const size_t defbase_length = LIB_listbase_count(defbase);

	deform_params.pose_channel_by_vertex_group.reinitialize(defbase_length);

	size_t index;
	LISTBASE_FOREACH_INDEX(DeformGroup *, dg, defbase, index) {
		PoseChannel *pchannel = KER_pose_channel_find_name(obarmature->pose, dg->name);

		deform_params.pose_channel_by_vertex_group[index] = (pchannel && !(pchannel->bone->flag & BONE_NO_DEFORM)) ? pchannel : NULL;
	}

	deform_params.armature_to_target = float4x4(KER_object_world_to_object(obtarget)) * float4x4(KER_object_object_to_world(obarmature));
	deform_params.target_to_armature = float4x4(KER_object_world_to_object(obarmature)) * float4x4(KER_object_object_to_world(obtarget));

	return deform_params;
}

ROSE_STATIC GPUVertFormat *extract_weights_format() {
	static GPUVertFormat format = {0};
	if (GPU_vertformat_empty(&format)) {
		GPU_vertformat_add(&format, "defgroup", GPU_COMP_I32, 4, GPU_FETCH_INT);
		GPU_vertformat_add(&format, "weight", GPU_COMP_F32, 4, GPU_FETCH_FLOAT);
	}
	return &format;
}

struct MDeformDeviceData {
	int4 defgroup = int4(-1, -1, -1, -1);
	float4 weight = float4(0, 0, 0, 0);
};

void extract_weights_mesh_ubo(const Object *obarmature, const Object *obtarget, const Mesh *metarget, rose::Vector<float4x4>& ubo_data) {
	const ListBase *defbase = NULL;
	if (metarget) {
		defbase = KER_id_defgroup_list_get(&metarget->id);
	}
	else {
		defbase = KER_id_defgroup_list_get(&obtarget->id);
	}

	/* gather the deform matrices for each vertex group. */

	ArmatureDeviceDeformParams params = get_armature_device_deform_params(obarmature, obtarget, defbase);

	ubo_data.resize(params.pose_channel_by_vertex_group.size());

	rose::threading::parallel_for(params.pose_channel_by_vertex_group.index_range(), 64, [&](const rose::IndexRange range) {
		const rose::IndexRange def_nr_range = params.pose_channel_by_vertex_group.index_range();

		for (const size_t group : range) {
			const PoseChannel *pchannel = def_nr_range.contains(group) ? params.pose_channel_by_vertex_group[group] : NULL;

			if (pchannel == NULL) {
				continue;
			}

			ubo_data[group] = float4x4(pchannel->chan_mat);
		}
	});
}

void extract_weights_mesh_vbo(const Object *obarmature, const Object *obtarget, const Mesh *metarget, rose::MutableSpan<MDeformDeviceData> vbo_data) {
	rose::Span<MDeformVert> dverts = KER_mesh_deform_verts_span(metarget);
	rose::Span<int> vcorners = KER_mesh_corner_verts_span(metarget);

	if (dverts.is_empty()) {
		vbo_data.fill(MDeformDeviceData());
		return;
	}

	/* gather the deform vertices for each vertex. */

	rose::threading::parallel_for(vcorners.index_range(), 1024, [&](const rose::IndexRange range) {
		for (const size_t corner : range) {
			const MDeformVert *dvert = &dverts[vcorners[corner]];

			vbo_data[corner] = MDeformDeviceData();

			const rose::Span<MDeformWeight> dweights(dvert->dw, dvert->totweight);
			for (const size_t index : dweights.index_range()) {
				if (index >= 4) {
					break;
				}

				vbo_data[corner].defgroup[index] = dweights[index].def_nr;
				vbo_data[corner].weight[index] = dweights[index].weight;
			}
		}
	});
}

GPUUniformBuf *extract_weights(const Object *obarmature, const Object *obtarget, const Mesh *mesh, GPUVertBuf *vbo) {
	GPU_vertbuf_init_with_format(vbo, extract_weights_format());
	GPU_vertbuf_data_alloc(vbo, mesh->totloop);

	MDeformDeviceData *data = static_cast<MDeformDeviceData *>(GPU_vertbuf_get_data(vbo));
	rose::MutableSpan<MDeformDeviceData> vbo_data = rose::MutableSpan<MDeformDeviceData>(data, mesh->totloop);

	rose::Vector<float4x4> ubo_data;

	extract_weights_mesh_vbo(obarmature, obtarget, mesh, vbo_data);
	extract_weights_mesh_ubo(obarmature, obtarget, mesh, ubo_data);

	return GPU_uniformbuf_create_ex(ubo_data.size_in_bytes(), &ubo_data[0], "Armature");
}
