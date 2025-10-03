#include "MEM_guardedalloc.h"

#include "DNA_meshdata_types.h"

#include "DRW_cache.h"

#include "KER_mesh.hh"

#include "GPU_index_buffer.h"
#include "GPU_vertex_buffer.h"

#include "LIB_array_utils.hh"
#include "LIB_assert.h"
#include "LIB_listbase.h"
#include "LIB_math_geom.h"
#include "LIB_math_vector_types.hh"
#include "LIB_task.hh"
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

ROSE_STATIC void extract_triangles_mesh(const Mesh *mesh, GPUIndexBuf *ibo) {
	const int triangles = poly_to_tri_count(mesh->totpoly, mesh->totloop);

	GPUIndexBufBuilder builder;
	GPU_indexbuf_init(&builder, GPU_PRIM_TRIS, triangles, mesh->totloop);
	
	rose::MutableSpan<uint3> write = rose::MutableSpan<uint3>(reinterpret_cast<uint3 *>(GPU_indexbuf_get_data(&builder)), triangles);
	rose::Span<uint3> read = rose::Span<uint3>(reinterpret_cast<const uint3 *>(KER_mesh_looptris(mesh)), triangles);

	/**
	 * We copy the array in paraller!
	 */
	rose::array_utils::gather(read, read.index_range(), write);

	GPU_indexbuf_build_in_place_ex(&builder, 0, triangles, false, ibo);
}

void extract_triangles(const Mesh *mesh, GPUIndexBuf *ibo) {
	GPU_indexbuf_init_build_on_device(ibo, poly_to_tri_count(mesh->totpoly, mesh->totloop) * 3);

	/**
	 * We are to build the cache #triangles index buffer as a subrange of the total!
	 */

	extract_triangles_mesh(mesh, ibo);
}
