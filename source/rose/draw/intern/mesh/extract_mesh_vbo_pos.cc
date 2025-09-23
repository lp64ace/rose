#include "MEM_guardedalloc.h"

#include "DNA_meshdata_types.h"

#include "DRW_cache.h"

#include "KER_mesh.hh"

#include "GPU_index_buffer.h"
#include "GPU_vertex_buffer.h"

#include "LIB_assert.h"
#include "LIB_array_utils.hh"
#include "LIB_math_vector_types.hh"
#include "LIB_listbase.h"
#include "LIB_span.hh"
#include "LIB_utildefines.h"

#include "extract_mesh.h"

ROSE_STATIC GPUVertFormat *extract_positions_format() {
	static GPUVertFormat format = {0};
	if (GPU_vertformat_empty(&format)) {
		GPU_vertformat_add(&format, "pos", GPU_COMP_F32, 3, GPU_FETCH_FLOAT);
	}
	return &format;
}

ROSE_STATIC void extract_positions_mesh(const Mesh *mesh, rose::MutableSpan<float3> data) {
	rose::Span<float3> vpositions = KER_mesh_vert_positions_span(mesh);
	rose::Span<int> vcorners = KER_mesh_corner_verts_span(mesh);

	/**
	 * Should we even bother to copy the loose geometry too?
	 */
	rose::array_utils::gather(vpositions, vcorners, data);
}

void extract_positions(const Mesh *mesh, GPUVertBuf *vbo) {
	GPU_vertbuf_init_with_format(vbo, extract_positions_format());
	GPU_vertbuf_data_alloc(vbo, mesh->totloop);

	rose::MutableSpan<float3> vbo_data = rose::MutableSpan<float3>(static_cast<float3 *>(GPU_vertbuf_get_data(vbo)), mesh->totloop);
	/**
	 * We could add implementation for rendering RMesh structures directly!
	 * @todo; Only in case we need to avoid conversion to Mesh.
	 */
	extract_positions_mesh(mesh, vbo_data);
}
