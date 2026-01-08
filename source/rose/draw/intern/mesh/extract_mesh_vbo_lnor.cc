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

ROSE_STATIC GPUVertFormat *extract_normals_format_high_quality() {
	static GPUVertFormat format = {0};
	if (GPU_vertformat_empty(&format)) {
		GPU_vertformat_add(&format, "nor", GPU_COMP_I16, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);
		GPU_vertformat_alias_add(&format, "lnor");
	}
	return &format;
}

ROSE_STATIC GPUVertFormat *extract_normals_format() {
	static GPUVertFormat format = {0};
	if (GPU_vertformat_empty(&format)) {
		GPU_vertformat_add(&format, "nor", GPU_COMP_I10, 4, GPU_FETCH_INT_TO_FLOAT_UNIT);
		GPU_vertformat_alias_add(&format, "lnor");
	}
	return &format;
}

template<typename T> inline T convert_normal(const float3 &src);

template<> inline GPUPackedNormal convert_normal(const float3 &src) {
	GPUNormal normal;
	GPU_normal_convert_v3(&normal, &src[0], false);
	return normal.low;
}

template<> inline short4 convert_normal(const float3 &src) {
	GPUNormal normal;
	GPU_normal_convert_v3(&normal, &src[0], true);
	return normal.high;
}

/** Her for debugging the normal calculation */
#define DRAW_POLY_NORMALS 0
#define DRAW_VERT_NORMALS 0

template<typename T> ROSE_STATIC void extract_normals_mesh(const Mesh *mesh, rose::MutableSpan<T> normals) {

#if DEBUG_POLY_NORMALS
	rose::Span<float3> poly_normals = KER_mesh_poly_normals_span(mesh);
	rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);
	rose::OffsetIndices<int> polys = KER_mesh_poly_offsets_span(mesh);

	/**
	 * Should we even bother to copy the loose geometry too?
	 */
	rose::threading::parallel_for(polys.index_range(), 2048, [&](const rose::IndexRange range) {
		/**
		 * These are the poly normals indexed per loop, "flat" normals!
		 *
		 * Each polygon in #polys[poly_i] contains a range of loops [polys[poly_i], polys[poly_i] + 1],
		 * we iterate for each of these loops and we copy the normals from the poly there.
		 */
		for (const int poly_i : range) {
			T n = convert_normal<T>(poly_normals[poly_i]);

			for (const int loop_i : polys[poly_i]) {
				normals[loop_i] = n;
			}
		}
	});

	return;
#endif

#if DEBUG_VERT_NORMALS
	rose::Span<float3> vert_normals = KER_mesh_vert_normals_span(mesh);
	rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);

	/**
	 * Should we even bother to copy the loose geometry too?
	 */
	rose::threading::parallel_for(corner_verts.index_range(), 2048, [&](const rose::IndexRange range) {
		/**
		 * These are the poly normals indexed per loop, "flat" normals!
		 *
		 * Each polygon in #polys[poly_i] contains a range of loops [polys[poly_i], polys[poly_i] + 1],
		 * we iterate for each of these loops and we copy the normals from the poly there.
		 */
		for (const int vert_i : range) {
			normals[vert_i] = convert_normal<T>(vert_normals[corner_verts[vert_i]]);
		}
	});

	return;
#endif

	rose::Span<float3> corner_normals = KER_mesh_corner_normals_span(mesh);

	rose::threading::parallel_for(corner_normals.index_range(), 1024, [&](const rose::IndexRange range) {
		for (size_t i : range) {
			normals[i] = convert_normal<T>(corner_normals[i]);
		}
	});
}

void extract_normals(const Mesh *mesh, GPUVertBuf *vbo, bool use_hq) {
	int size = mesh->totloop;

	if (use_hq) {
		GPUVertFormat *format = extract_normals_format_high_quality();
		GPU_vertbuf_init_with_format(vbo, format);
		GPU_vertbuf_data_alloc(vbo, size);

		rose::MutableSpan vbo_data = rose::MutableSpan(static_cast<short4 *>(GPU_vertbuf_get_data(vbo)), mesh->totloop);

		extract_normals_mesh(mesh, vbo_data);
	}
	else {
		GPUVertFormat *format = extract_normals_format();
		GPU_vertbuf_init_with_format(vbo, format);
		GPU_vertbuf_data_alloc(vbo, size);

		rose::MutableSpan vbo_data = rose::MutableSpan(static_cast<GPUPackedNormal *>(GPU_vertbuf_get_data(vbo)), mesh->totloop);

		extract_normals_mesh(mesh, vbo_data);
	}
}
