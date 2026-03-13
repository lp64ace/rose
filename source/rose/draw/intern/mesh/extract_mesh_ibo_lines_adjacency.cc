#include "MEM_guardedalloc.h"

#include "DNA_meshdata_types.h"

#include "DRW_cache.h"

#include "KER_mesh.hh"

#include "GPU_index_buffer.h"
#include "GPU_vertex_buffer.h"

#include "LIB_array_utils.hh"
#include "LIB_assert.h"
#include "LIB_ghash.h"
#include "LIB_hash.h"
#include "LIB_listbase.h"
#include "LIB_math_geom.h"
#include "LIB_math_vector_types.hh"
#include "LIB_span.hh"
#include "LIB_task.hh"
#include "LIB_utildefines.h"

#include "extract_mesh.h"

#include "intern/draw_cache_private.h"

/* ---------------------------------------------------------------------- */
/** \name Extract Line Adjacency Indices (Shadow Volume)
 * \{ */

#define NO_EDGE INT_MAX

/**
 * Packed edge key for the hash - stores v_lo in low 32 bits, v_hi in high 32 bits.
 * We always sort so v_lo <= v_hi.
 */
ROSE_STATIC uint64_t edge_key(uint v0, uint v1) {
	uint lo = rose::math::min(v0, v1);
	uint hi = rose::math::max(v0, v1);
	return ((uint64_t)hi << 32) | (uint64_t)lo;
}

/**
 * Per-edge state stored in the hash during triangle iteration.
 *   - value == NO_EDGE     -> edge was matched (manifold), already emitted
 *   - value > 0            -> l1 + 1, winding NOT inverted
 *   - value < 0            -> -(l1 + 1), winding inverted
 */
struct LineAdjacencyData {
	GPUIndexBufBuilder builder;
	
	/**
	 * Maps vertex index -> any loop index touching that vertex.
	 * Used to recover l2/l3 for non-manifold boundary edges at finish.
	 */
	rose::Array<unsigned int> vert_to_loop = {};
	rose::Map<uint64_t, int> edge_map = {};

	bool is_manifold = false;
};

ROSE_STATIC void lines_adjacency_triangle(uint v1, uint v2, uint v3, uint l1, uint l2, uint l3, LineAdjacencyData *data) {
	GPUIndexBufBuilder *elb = &data->builder;

	/* Rotate through all three edges of the triangle. */
	for (int e = 0; e < 3; e++) {
		/**
		 * Rotate: (v1,v2,v3) -> (v2,v3,v1) each iteration.
		 * After rotation: current edge is (v2, v3), opposite vert is v1.
		 */
		uint tv = v1;
		v1 = v2;
		v2 = v3;
		v3 = tv;
		uint tl = l1;
		l1 = l2;
		l2 = l3;
		l3 = tl;

		const bool inv_indices = (v2 > v3);
		const uint64_t key = edge_key(v2, v3);

		int &slot = data->edge_map.lookup_or_add(key, 0);

		if (slot == 0 || slot == NO_EDGE) {
			/* First triangle on this edge - store winding + loop.
			 * +1 offset because 0 is ambiguous with uninitialized. */
			int value = (int)l1 + 1;
			slot = inv_indices ? -value : value;

			/* Record vert -> loop for non-manifold fallback at finish. */
			data->vert_to_loop[v2] = l2;
			data->vert_to_loop[v3] = l3;
		}
		else {
			/* Second triangle - emit adjacency and mark edge done. */
			const bool inv_opposite = (slot < 0);
			const uint l_opposite = (uint)rose::math::abs(slot) - 1;

			/* Tag as consumed. */
			slot = NO_EDGE;

			if (inv_opposite == inv_indices) {
				/* Non-manifold: winding mismatch - emit degenerate (no shared opposite). */
				GPU_indexbuf_add_line_adj_verts(elb, l1, l2, l3, l1);
				GPU_indexbuf_add_line_adj_verts(elb, l_opposite, l2, l3, l_opposite);
				data->is_manifold = false;
			}
			else {
				/* Manifold interior edge - emit shared adjacency. */
				GPU_indexbuf_add_line_adj_verts(elb, l1, l2, l3, l_opposite);
			}
		}
	}
}

void extract_lines_adjacency(MeshBatchCache *cache, const Mesh *mesh, GPUIndexBuf *ibo) {
	const int triangles = poly_to_tri_count(mesh->totpoly, mesh->totloop);
	const int tesselate_edges = mesh->totloop + triangles - mesh->totpoly;

	LineAdjacencyData data;
	data.is_manifold = true;
	data.vert_to_loop = rose::Array<uint>(mesh->totvert, 0);
	GPU_indexbuf_init(&data.builder, GPU_PRIM_LINES_ADJ, tesselate_edges, mesh->totloop);

	rose::Span<uint3> looptris = rose::Span<uint3>(reinterpret_cast<const uint3 *>(KER_mesh_looptris(mesh)), triangles);
	rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);

	/* Single pass over all looptris - O(T) where T = triangle count. */
	for (const uint3 &tri : looptris) {
		const uint l0 = tri[0], l1 = tri[1], l2 = tri[2];
		lines_adjacency_triangle((uint)corner_verts[l0], (uint)corner_verts[l1], (uint)corner_verts[l2], l0, l1, l2, &data);
	}

	/* Finish - emit all unmatched (boundary / non-manifold) edges. */
	for (auto item : data.edge_map.items()) {
		const int v_data = item.value;
		if (v_data == NO_EDGE || v_data == 0) {
			continue;
		}

		/* Recover original winding to get correct l2/l3. */
		uint v2, v3;
		v2 = (uint)(item.key & 0xFFFFFFFF);
		v3 = (uint)(item.key >> 32);

		const uint l1 = (uint)rose::math::abs(v_data) - 1;
		if (v_data < 0) {
			SWAP(uint, v2, v3);
		}

		const uint l2 = data.vert_to_loop[v2];
		const uint l3 = data.vert_to_loop[v3];

		GPU_indexbuf_add_line_adj_verts(&data.builder, l1, l2, l3, l1);
		data.is_manifold = false;
	}

	cache->is_manifold = data.is_manifold;

	GPU_indexbuf_build_in_place(&data.builder, ibo);
}

#undef NO_EDGE

/** \} */
