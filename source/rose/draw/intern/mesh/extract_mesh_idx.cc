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

ROSE_STATIC GPUVertFormat *extract_select_idx_format() {
	static GPUVertFormat format = {0};
	if (GPU_vertformat_empty(&format)) {
		GPU_vertformat_add(&format, "index", GPU_COMP_I32, 1, GPU_FETCH_INT);
	}
	return &format;
}

void extract_poly_idx_mesh(const Mesh *mesh, rose::MutableSpan<int> data) {
	// TODO!
}

void extract_edge_idx_mesh(const Mesh *mesh, rose::MutableSpan<int> data) {
	// TODO!
}

void extract_vert_idx_mesh(const Mesh *mesh, rose::MutableSpan<int> data) {
	// TODO!
}

void extract_poly_idx(const Mesh *mesh, GPUVertBuf *vbo) {
	GPU_vertbuf_init_with_format(vbo, extract_select_idx_format());
	GPU_vertbuf_data_alloc(vbo, mesh->totloop);

	rose::MutableSpan<int> vbo_data = rose::MutableSpan<int>(static_cast<int *>(GPU_vertbuf_get_data(vbo)), mesh->totloop);
	/**
	 * We could add implementation for rendering RMesh structures directly!
	 * @todo; Only in case we need to avoid conversion to Mesh.
	 */
	extract_poly_idx_mesh(mesh, vbo_data);
}
