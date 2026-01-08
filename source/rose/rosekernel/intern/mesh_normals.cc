#include "MEM_guardedalloc.h"

#include "LIB_array_utils.hh"
#include "LIB_math_base.hh"
#include "LIB_math_geom.h"
#include "LIB_math_matrix.h"
#include "LIB_math_matrix.hh"
#include "LIB_math_vector.h"
#include "LIB_math_vector.hh"
#include "LIB_task.hh"
#include "LIB_offset_span.hh"
#include "LIB_offset_indices.hh"
#include "LIB_vector_set.hh"

#include "KER_mesh_types.hh"
#include "KER_mesh.hh"

#include "atomic_ops.h"

/* -------------------------------------------------------------------- */
/** \name Private Utility Functions
 * \{ */

/**
 * A thread-safe version of #add_v3_v3 that uses a spin-lock.
 *
 * \note Avoid using this when the chance of contention is high.
 */
static void add_v3_v3_atomic(float r[3], const float a[3]) {
#define FLT_EQ_NONAN(_fa, _fb) (*((const uint32_t *)&_fa) == *((const uint32_t *)&_fb))

	float virtual_lock = r[0];
	while (true) {
		/* This loops until following conditions are met:
		 * - `r[0]` has same value as virtual_lock (i.e. it did not change since last try).
		 * - `r[0]` was not `FLT_MAX`, i.e. it was not locked by another thread. */
		const float test_lock = atomic_cas_float(&r[0], virtual_lock, FLT_MAX);
		if (_ATOMIC_LIKELY(FLT_EQ_NONAN(test_lock, virtual_lock) && (test_lock != FLT_MAX))) {
			break;
		}
		virtual_lock = test_lock;
	}
	virtual_lock += a[0];
	r[1] += a[1];
	r[2] += a[2];

	/* Second atomic operation to 'release'
	 * our lock on that vector and set its first scalar value. */
	/* Note that we do not need to loop here, since we 'locked' `r[0]`,
	 * nobody should have changed it in the mean time. */
	virtual_lock = atomic_cas_float(&r[0], FLT_MAX, virtual_lock);
	ROSE_assert(virtual_lock == FLT_MAX);

#undef FLT_EQ_NONAN
}

/* \} */

namespace rose::kernel::mesh {

ROSE_STATIC float3 normal_calc_ngon(const Span<float3> vert_positions, const Span<int> poly_verts) {
	float3 normal(0);

	/* Newell's Method */
	const float *v_prev = vert_positions[poly_verts.last()];
	for (const int i : poly_verts.index_range()) {
		const float *v_curr = vert_positions[poly_verts[i]];
		add_newell_cross_v3_v3v3(normal, v_prev, v_curr);
		v_prev = v_curr;
	}

	if (normalize_v3(normal) == 0.0f) {
		normal[2] = 1.0f; /* other axis set to 0.0 */
	}

	return normal;
}

ROSE_STATIC float3 poly_normal_calc(const Span<float3> vert_positions, const Span<int> poly_verts) {
	if (poly_verts.size() > 4) {
		return normal_calc_ngon(vert_positions, poly_verts);
	}
	if (poly_verts.size() == 3) {
		return math::normal_tri(vert_positions[poly_verts[0]], vert_positions[poly_verts[1]], vert_positions[poly_verts[2]]);
	}
	if (poly_verts.size() == 4) {
		float3 normal;
		normal_quad_v3(normal, vert_positions[poly_verts[0]], vert_positions[poly_verts[1]], vert_positions[poly_verts[2]], vert_positions[poly_verts[3]]);
		return normal;
	}
	/* horrible, two sided face! */
	return float3(0);
}

void normals_calc_polys(const Span<float3> positions, const OffsetIndices<int> polys, const Span<int> corner_verts, MutableSpan<float3> poly_normals) {
	ROSE_assert(polys.size() == poly_normals.size());
	threading::parallel_for(polys.index_range(), 1024, [&](const IndexRange range) {
		for (const int i : range) {
			poly_normals[i] = poly_normal_calc(positions, corner_verts.slice(polys[i]));
		}
	});
}

void normals_calc_verts(const Span<float3> positions, const OffsetIndices<int> polys, const Span<int> corner_verts, const GroupedSpan<int> vert_to_face_map, const Span<float3> poly_normals, MutableSpan<float3> vert_normals) {
	threading::parallel_for(positions.index_range(), 1024, [&](const IndexRange range) {
		for (const int vert_i : range) {
			const Span<int> vert_faces = vert_to_face_map[vert_i];
			if (vert_faces.is_empty()) {
				vert_normals[vert_i] = math::normalize(positions[vert_i]);
				continue;
			}

			float3 vert_normal(0);
			for (const int poly_i : vert_faces) {
				const int2 adjacent_verts = mesh::face_find_adjacent_verts(polys[poly_i], corner_verts, vert_i);
				const float3 dir_prev = math::normalize(positions[adjacent_verts[0]] - positions[vert_i]);
				const float3 dir_next = math::normalize(positions[adjacent_verts[1]] - positions[vert_i]);
				const float factor = math::safe_acos(math::dot(dir_prev, dir_next));

				vert_normal += poly_normals[poly_i] * factor;
			}

			vert_normals[vert_i] = math::normalize(vert_normal);
		}
	});
}

struct VertCornerInfo {
	int face;
	int corner;
	int corner_prev;
	int corner_next;
	int vert_prev;
	int vert_next;
	int local_edge_prev;
	int local_edge_next;
};

/**
 * Gather data related to all the connected faces / face corners. This makes accessing it simpler
 * later on in the various per-vertex hot loops. It also means we can be sure it will be in CPU
 * caches. Gathering it into a single Vector of an "info" struct rather than multiple vectors is
 * expected to be worth it because there are typically very few connected corners; the overhead of
 * a Vector for each piece of data would be significant.
 */
static void collect_corner_info(const OffsetIndices<int> faces, const Span<int> corner_verts, const Span<int> vert_faces, const int vert, MutableSpan<VertCornerInfo> r_corner_infos) {
	for (const int i : vert_faces.index_range()) {
		const int face = vert_faces[i];
		r_corner_infos[i].face = face;
		r_corner_infos[i].corner = face_find_corner_from_vert(faces[face], corner_verts, vert);
		r_corner_infos[i].corner_prev = face_corner_prev(faces[face], r_corner_infos[i].corner);
		r_corner_infos[i].corner_next = face_corner_next(faces[face], r_corner_infos[i].corner);
		r_corner_infos[i].vert_prev = corner_verts[r_corner_infos[i].corner_prev];
		r_corner_infos[i].vert_next = corner_verts[r_corner_infos[i].corner_next];
	}
}

/** The edge hasn't been handled yet while the edge info is being created. */
using EdgeUninitialized = std::monostate;

/**
 * The first corner has been added to the edge. For boundary edges, this is the only corner. We
 * store whether the winding direction of the face was towards or away from the vertex to be able
 * to detect when the winding direction of two neighboring faces doesn't match.
 */
struct EdgeOneCorner {
  int local_corner_1;
  bool winding_torwards_vert;
};

/**
 * The edge is manifold and is used by two faces/corners. The actual faces and corners have to be
 * retrieved with the data in #VertCornerInfo.
 */
struct EdgeTwoCorners {
  int local_corner_1;
  int local_corner_2;
};

/**
 * The edge "breaks" the topology flow of faces around the vertex. It could be marked sharp
 * explicitly, it could be used by a sharp face, it could have mismatched face winding directions,
 * or it might be non-manifold and used by more than two faces.
 */
struct EdgeSharp {};

using VertEdgeInfo = std::variant<EdgeUninitialized, EdgeOneCorner, EdgeTwoCorners, EdgeSharp>;

static void add_corner_to_edge(const Span<int> corner_edges, const Span<bool> sharp_edges, const int local_corner, const int corner, const int other_corner, const bool winding_torwards_vert, VertEdgeInfo &info) {
	if (std::holds_alternative<EdgeUninitialized>(info)) {
		if (!sharp_edges.is_empty()) {
			/* The first time we encounter the edge, we check if it is marked sharp. In that case corner
			 * fans shouldn't propagate past it. To find the edge we need to check if the current corner
			 * references the edge connected to `other_corner` or if `other_corner` uses the edge. */
			if (sharp_edges[corner_edges[winding_torwards_vert ? other_corner : corner]]) {
				info = EdgeSharp{};
				return;
			}
		}
		info = EdgeOneCorner{local_corner, winding_torwards_vert};
	}
	else if (const EdgeOneCorner *info_one_edge = std::get_if<EdgeOneCorner>(&info)) {
		/* If the edge ends up being used by faces, we still have to check if the winding direction
		 * changes. Though it's an undesirable situation for the mesh to be in, we shouldn't propagate
		 * smooth normals across edges facing opposite directions. Breaking the flow on these winding
		 * direction changes also simplifies the fan traversal later on; without it the we couldn't
		 * traverse by just continuing to use the next/previous corner. */
		if (info_one_edge->winding_torwards_vert == winding_torwards_vert) {
			info = EdgeSharp{};
			return;
		}
		info = EdgeTwoCorners{info_one_edge->local_corner_1, local_corner};
	}
	else {
		/* The edge is either already sharp, or we're trying to add a third corner. */
		info = EdgeSharp{};
	}
}

/** Use a custom VectorSet type to use int32 instead of int64 for the key indices. */
using LocalEdgeVectorSet = VectorSet<int, DefaultProbingStrategy, DefaultHash<int>, DefaultEquality<int>, SimpleVectorSetSlot<int>, GuardedAllocator>;

/**
 * Create a local indexing for the edges connected to the vertex (not including loose edges of
 * course). We could look up the edge indices from the VectorSet as necessary later, but it should
 * be better to just use a bit more space in #VertCornerInfo to simplify things instead.
 */
static void calc_local_edge_indices(MutableSpan<VertCornerInfo> corner_infos, LocalEdgeVectorSet &r_other_vert_to_edge) {
	r_other_vert_to_edge.reserve(corner_infos.size());
	for (VertCornerInfo &info : corner_infos) {
		info.local_edge_prev = r_other_vert_to_edge.index_of_or_add(info.vert_prev);
		info.local_edge_next = r_other_vert_to_edge.index_of_or_add(info.vert_next);
	}
}

static void calc_connecting_edge_info(const Span<int> corner_edges, const Span<bool> sharp_edges, const Span<bool> sharp_faces, const Span<VertCornerInfo> corner_infos, MutableSpan<VertEdgeInfo> edge_infos) {
	for (const int local_corner : corner_infos.index_range()) {
		const VertCornerInfo &info = corner_infos[local_corner];
		if (!sharp_faces.is_empty() && sharp_faces[info.face]) {
			/* Sharp faces implicitly cause sharp edges. */
			edge_infos[info.local_edge_prev] = EdgeSharp{};
			edge_infos[info.local_edge_next] = EdgeSharp{};
			continue;
		}
		/* The "previous" edge is winding towards the vertex, the "next" edge is winding away. */
		add_corner_to_edge(corner_edges, sharp_edges, local_corner, info.corner, info.corner_prev, true, edge_infos[info.local_edge_prev]);
		add_corner_to_edge(corner_edges, sharp_edges, local_corner, info.corner, info.corner_next, false, edge_infos[info.local_edge_next]);
	}
}

/**
 * From a starting corner, follow the connected edges to find the other corners "fanning" around
 * the vertex. Crucially, we've removed ambiguity from the process already by marking edges
 * connected to three faces and edges between faces with opposite winding direction sharp.
 */
static void traverse_fan_local_corners(const Span<VertCornerInfo> corner_infos, const Span<VertEdgeInfo> edge_infos, const int start_local_corner, Vector<int, 16> &result_fan) {
	result_fan.append(start_local_corner);
	{
		/* Travel around the vertex in a right-handed clockwise direction (based on the normal). The
		 * corners found in this traversal are reversed so the direction matches with the next
		 * traversal (or so that the next traversal doesn't have to be added at the beginning of the
		 * vector). */
		int current = start_local_corner;
		int local_edge = corner_infos[current].local_edge_next;
		bool found_cyclic_fan = false;
		while (const EdgeTwoCorners *edge = std::get_if<EdgeTwoCorners>(&edge_infos[local_edge])) {
			current = mesh::edge_other_vert(int2(edge->local_corner_1, edge->local_corner_2), current);
			if (current == start_local_corner) {
				found_cyclic_fan = true;
				break;
			}
			result_fan.append(current);
			local_edge = corner_infos[current].local_edge_next;
		}
		/* Reverse the corners added so the final order is consistent with the next traversal. */
		result_fan.as_mutable_span().reverse();

		if (found_cyclic_fan) {
			/* To match behavior from the previous implementation of face corner normal calculation, the
			 * final fan is rotated so that the smallest face corner index comes first. */
			int *fan_first_corner = std::min_element(result_fan.begin(), result_fan.end(), [&](const int a, const int b) { return corner_infos[a].corner < corner_infos[b].corner; });
			std::rotate(result_fan.begin(), fan_first_corner, result_fan.end());
			return;
		}
	}

	/* Travel in the other direction. */
	int current = start_local_corner;
	int local_edge = corner_infos[current].local_edge_prev;
	while (const EdgeTwoCorners *edge = std::get_if<EdgeTwoCorners>(&edge_infos[local_edge])) {
		current = current == edge->local_corner_1 ? edge->local_corner_2 : edge->local_corner_1;
		/* Cyclic fans have already been found, so there's no need to check for them here. */
		result_fan.append(current);
		local_edge = corner_infos[current].local_edge_prev;
	}
}

/**
 * The edge directions are used to compute factors for the face normals from each corner. Since
 * they involve a normalization it's worth it to compute them once, especially since we've
 * deduplicated the edge indices and can easily index them with #VertCornerInfo.
 */
static void calc_edge_directions(const Span<float3> vert_positions, const Span<int> local_edge_by_vert, const float3 &vert_position, MutableSpan<float3> edge_dirs) {
	for (const int i : local_edge_by_vert.index_range()) {
		edge_dirs[i] = math::normalize(vert_positions[local_edge_by_vert[i]] - vert_position);
	}
}

/** The normal for all the corners in the fan is a weighted combination of their face normals. */
static float3 accumulate_fan_normal(const Span<VertCornerInfo> corner_infos, const Span<float3> edge_dirs, const Span<float3> face_normals, const Span<int> local_corners_in_fan) {
	if (local_corners_in_fan.size() == 1) {
		/* Logically this special case is unnecessary, but due to floating point precision it is
		 * required for the output to be the same as previous versions of the algorithm. */
		return face_normals[corner_infos[local_corners_in_fan.first()].face];
	}
	float3 fan_normal(0);
	for (const int local_corner : local_corners_in_fan) {
		const VertCornerInfo &info = corner_infos[local_corner];
		const float3 &dir_prev = edge_dirs[info.local_edge_prev];
		const float3 &dir_next = edge_dirs[info.local_edge_next];
		const float factor = math::safe_acos(math::dot(dir_prev, dir_next));
		fan_normal += face_normals[info.face] * factor;
	}
	return math::normalize(fan_normal);
}


void normals_calc_corners(const Span<float3> vert_positions, const OffsetIndices<int> polys, const Span<int> corner_verts, const Span<int> corner_edges, const GroupedSpan<int> vert_to_face_map, const Span<float3> poly_normals, const Span<bool> sharp_edges, const Span<bool> sharp_faces, MutableSpan<float3> corner_normals) {
	/* Mesh is not empty, but there are no faces, so no normals. */
	if (corner_verts.is_empty()) {
		return;
	}

	threading::parallel_for(vert_positions.index_range(), 256, [&](const IndexRange range) {
		Vector<VertCornerInfo, 16> corner_infos;
		LocalEdgeVectorSet local_edge_by_vert;
		Vector<VertEdgeInfo, 16> edge_infos;
		Vector<float3, 16> edge_dirs;
		Vector<bool, 16> local_corner_visited;
		Vector<int, 16> corners_in_fan;

		for (const int vert : range) {
			const float3 vert_position = vert_positions[vert];
			const Span<int> vert_polys = vert_to_face_map[vert];

			/* Because we're iterating over vertices in order to batch work for their connected face
			 * corners, we have to handle loose vertices and vertices not used by faces. */
			if (vert_polys.is_empty()) {
				continue;
			}

			corner_infos.resize(vert_polys.size());
			collect_corner_info(polys, corner_verts, vert_polys, vert, corner_infos);

			local_edge_by_vert.clear();
			calc_local_edge_indices(corner_infos, local_edge_by_vert);

			edge_infos.clear();
			edge_infos.resize(local_edge_by_vert.size());
			calc_connecting_edge_info(corner_edges, sharp_edges, sharp_faces, corner_infos, edge_infos);

			edge_dirs.resize(edge_infos.size());
			calc_edge_directions(vert_positions, local_edge_by_vert, vert_position, edge_dirs);

			/* Though we are protected from traversing to the same corner twice by the fact that 3-way
			 * connections are marked sharp, we need to maintain the "visited" status of each corner so
			 * we can find the next start corner for each subsequent fan traversal. Keeping track of the
			 * number of visited corners is a quick way to avoid this book keeping for the final fan (and
			 * there are usually just two, so that should be worth it). */
			int visited_count = 0;
			local_corner_visited.resize(vert_polys.size());
			local_corner_visited.fill(false);

			int start_local_corner = 0;
			while (true) {
				corners_in_fan.clear();
				traverse_fan_local_corners(corner_infos, edge_infos, start_local_corner, corners_in_fan);

				float3 fan_normal = accumulate_fan_normal(corner_infos, edge_dirs, poly_normals, corners_in_fan);

				for (const int local_corner : corners_in_fan) {
					const VertCornerInfo &info = corner_infos[local_corner];
					corner_normals[info.corner] = fan_normal;
				}

				visited_count += corners_in_fan.size();
				if (visited_count == corner_infos.size()) {
					break;
				}

				local_corner_visited.as_mutable_span().fill_indices(corners_in_fan.as_span(), true);
				ROSE_assert(!local_corner_visited.as_span().take_front(start_local_corner).contains(false));
				ROSE_assert(local_corner_visited.as_span().drop_front(start_local_corner).contains(false));
				/* Will start traversing the next smooth fan mixed in shared index space. */
				while (local_corner_visited[start_local_corner]) {
					start_local_corner++;
				}
			}
			ROSE_assert(visited_count == corner_infos.size());
		}
	});
}

void normals_calc_poly_vert(const Span<float3> positions, const OffsetIndices<int> polys, const Span<int> corner_verts, MutableSpan<float3> poly_normals, MutableSpan<float3> vert_normals) {
	/* Zero the vertex normal array for accumulation. */
	memset(vert_normals.data(), 0, vert_normals.as_span().size_in_bytes());

	/* Compute poly normals, accumulating them into vertex normals. */
	{
		threading::parallel_for(polys.index_range(), 1024, [&](const IndexRange range) {
			for (const int poly_i : range) {
				const Span<int> poly_verts = corner_verts.slice(polys[poly_i]);

				float3 &pnor = poly_normals[poly_i];

				const int i_end = poly_verts.size() - 1;

				/* Polygon Normal and edge-vector. */
				/* Inline version of #poly_normal_calc, also does edge-vectors. */
				{
					zero_v3(pnor);
					/* Newell's Method */
					const float *v_curr = positions[poly_verts[i_end]];
					for (int i_next = 0; i_next <= i_end; i_next++) {
						const float *v_next = positions[poly_verts[i_next]];
						add_newell_cross_v3_v3v3(pnor, v_curr, v_next);
						v_curr = v_next;
					}
					if (normalize_v3(pnor) == 0.0f) {
						pnor[2] = 1.0f; /* Other axes set to zero. */
					}
				}

				/* Accumulate angle weighted face normal into the vertex normal. */
				/* Inline version of #accumulate_vertex_normals_poly_v3. */
				{
					float edvec_prev[3], edvec_next[3], edvec_end[3];
					const float *v_curr = positions[poly_verts[i_end]];
					sub_v3_v3v3(edvec_prev, positions[poly_verts[i_end - 1]], v_curr);
					normalize_v3(edvec_prev);
					copy_v3_v3(edvec_end, edvec_prev);

					for (int i_next = 0, i_curr = i_end; i_next <= i_end; i_curr = i_next++) {
						const float *v_next = positions[poly_verts[i_next]];

						/* Skip an extra normalization by reusing the first calculated edge. */
						if (i_next != i_end) {
							sub_v3_v3v3(edvec_next, v_curr, v_next);
							normalize_v3(edvec_next);
						}
						else {
							copy_v3_v3(edvec_next, edvec_end);
						}

						/* Calculate angle between the two poly edges incident on this vertex. */
						const float fac = saacos(-dot_v3v3(edvec_prev, edvec_next));
						const float vnor_add[3] = {pnor[0] * fac, pnor[1] * fac, pnor[2] * fac};

						float *vnor = vert_normals[poly_verts[i_curr]];
						add_v3_v3_atomic(vnor, vnor_add);
						v_curr = v_next;
						copy_v3_v3(edvec_prev, edvec_next);
					}
				}
			}
		});
	}

	/* Normalize and validate computed vertex normals. */
	{
		threading::parallel_for(positions.index_range(), 1024, [&](const IndexRange range) {
			for (const int vert_i : range) {
				float *no = vert_normals[vert_i];

				if (normalize_v3(no) == 0.0f) {
					/* Following Mesh convention; we use vertex coordinate itself for normal in this
					 * case. */
					normalize_v3_v3(no, positions[vert_i]);
				}
			}
		});
	}
}

}  // namespace rose::kernel::mesh

void KER_mesh_normals_tag_dirty(Mesh *mesh) {
	mesh->runtime->vert_normals_dirty = true;
	mesh->runtime->poly_normals_dirty = true;
}

bool KER_mesh_vertex_normals_are_dirty(const Mesh *mesh) {
	return mesh->runtime->vert_normals_dirty;
}

bool KER_mesh_poly_normals_are_dirty(const Mesh *mesh) {
	return mesh->runtime->poly_normals_dirty;
}

float (*KER_mesh_vert_normals_for_write(struct Mesh *mesh))[3] {
	mesh->runtime->vert_normals.reinitialize(mesh->totvert);
	return reinterpret_cast<float(*)[3]>(mesh->runtime->vert_normals.data());
}
float (*KER_mesh_poly_normals_for_write(struct Mesh *mesh))[3] {
	mesh->runtime->poly_normals.reinitialize(mesh->totpoly);
	return reinterpret_cast<float(*)[3]>(mesh->runtime->poly_normals.data());
}

const float (*KER_mesh_vert_normals_ensure(const struct Mesh *mesh))[3] {
	if (!mesh->runtime->vert_normals_dirty) {
		ROSE_assert(mesh->runtime->vert_normals.size() == mesh->totvert);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->vert_normals.data());
	}

	const rose::Span<float3> poly_normals = KER_mesh_poly_normals_span(mesh);

	std::lock_guard lock{mesh->runtime->normals_mutex};
	if (!mesh->runtime->vert_normals_dirty) {
		ROSE_assert(mesh->runtime->vert_normals.size() == mesh->totvert);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->vert_normals.data());
	}

	rose::threading::isolate_task([&]() {
		const rose::Span<float3> positions = KER_mesh_vert_positions_span(mesh);
		const rose::OffsetIndices<int> polys = KER_mesh_poly_offsets_span(mesh);
		const rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);
		const rose::GroupedSpan<int> vert_to_face = KER_mesh_vert_to_face_map_span(mesh);

		mesh->runtime->vert_normals.reinitialize(positions.size());
		rose::kernel::mesh::normals_calc_verts(positions, polys, corner_verts, vert_to_face, poly_normals, mesh->runtime->vert_normals);
		mesh->runtime->vert_normals_dirty = false;
	});

	return reinterpret_cast<const float(*)[3]>(mesh->runtime->vert_normals.data());
}
const float (*KER_mesh_poly_normals_ensure(const struct Mesh *mesh))[3] {
	using namespace rose;
	if (!mesh->runtime->poly_normals_dirty) {
		ROSE_assert(mesh->runtime->poly_normals.size() == mesh->totpoly);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->poly_normals.data());
	}

	std::lock_guard lock{mesh->runtime->normals_mutex};
	if (!mesh->runtime->poly_normals_dirty) {
		ROSE_assert(mesh->runtime->poly_normals.size() == mesh->totpoly);
		return reinterpret_cast<const float(*)[3]>(mesh->runtime->poly_normals.data());
	}

	/* Isolate task because a mutex is locked and computing normals is multi-threaded. */
	threading::isolate_task([&]() {
		const rose::Span<float3> positions = KER_mesh_vert_positions_span(mesh);
		const rose::OffsetIndices<int> polys = KER_mesh_poly_offsets_span(mesh);
		const rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);

		mesh->runtime->poly_normals.reinitialize(polys.size());
		kernel::mesh::normals_calc_polys(positions, polys, corner_verts, mesh->runtime->poly_normals);

		mesh->runtime->poly_normals_dirty = false;
	});

	return reinterpret_cast<const float(*)[3]>(mesh->runtime->poly_normals.data());
}
const float (*KER_mesh_corner_normals_ensure(const struct Mesh *mesh))[3] {
	if (mesh->totpoly == 0) {
		return {};
	}

	mesh->runtime->corner_normals_cache.ensure([&](rose::Vector<float3> &r_data) {
		r_data.reinitialize(mesh->totloop);

		rose::MutableSpan<float3> data(r_data);

		const bool *sharp_edges_ptr = reinterpret_cast<const bool *>(CustomData_get_layer_named(&mesh->edata, CD_PROP_BOOL, "sharp_edge"));
		const bool *sharp_faces_ptr = reinterpret_cast<const bool *>(CustomData_get_layer_named(&mesh->fdata, CD_PROP_BOOL, "sharp_face"));

		const rose::Span<bool> sharp_edges(sharp_edges_ptr, (sharp_edges_ptr) ? (size_t)mesh->totedge : (size_t)0);
		const rose::Span<bool> sharp_faces(sharp_faces_ptr, (sharp_faces_ptr) ? (size_t)mesh->totedge : (size_t)0);

		const rose::Span<float3> positions = KER_mesh_vert_positions_span(mesh);
		const rose::OffsetIndices<int> polys = KER_mesh_poly_offsets_span(mesh);
		const rose::Span<int> corner_verts = KER_mesh_corner_verts_span(mesh);
		const rose::Span<int> corner_edges = KER_mesh_corner_edges_span(mesh);
		const rose::GroupedSpan<int> vert_to_face = KER_mesh_vert_to_face_map_span(mesh);
		const rose::Span<float3> poly_normals = KER_mesh_poly_normals_span(mesh);

		rose::kernel::mesh::normals_calc_corners(positions, polys, corner_verts, corner_edges, vert_to_face, poly_normals, sharp_edges, sharp_faces, data);
	});

	const rose::Vector<float3> &corner_normals = mesh->runtime->corner_normals_cache.data();
	return reinterpret_cast<const float(*)[3]>(corner_normals.data());
}

void KER_mesh_clear_derived_normals(Mesh *mesh) {
	mesh->runtime->vert_normals.clear_and_shrink();
	mesh->runtime->poly_normals.clear_and_shrink();

	mesh->runtime->vert_normals_dirty = true;
	mesh->runtime->poly_normals_dirty = true;
}
