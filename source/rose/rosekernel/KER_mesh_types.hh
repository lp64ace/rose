#ifndef KER_MESH_TYPES_HH
#define KER_MESH_TYPES_HH

#include "DNA_meshdata_types.h"

#include "LIB_array.hh"
#include "LIB_math_vector_types.hh"
#include "LIB_implicit_sharing.hh"
#include "LIB_shared_cache.hh"
#include "LIB_span.hh"
#include "LIB_vector.hh"

#include <mutex>

namespace rose::kernel {

struct MeshRuntime {
	/** Needed to ensure some thread-safety during render data pre-processing. */
	std::mutex render_mutex = {};

	/** Implicit sharing user count for #Mesh::poly_offset_indices. */
	const ImplicitSharingInfo *poly_offsets_sharing_info = nullptr;

	/** Cache for derived triangulation of the mesh, accessed with #Mesh::looptris(). */
	SharedCache<Array<MLoopTri>> looptris_cache = {};
	SharedCache<Array<int>> looptri_polys_cache = {};

	/**
	 * Caches for lazily computed vertex and polygon normals. These are stored here rather than in
	 * #CustomData because they can be calculated on a `const` mesh, and adding custom data layers
	 * on a `const` mesh is not thread-safe.
	 */

	/** Lazily computed face corner normals (#KER_mesh_corner_normals()). */
	SharedCache<Vector<float3>> vert_normals_cache = {};
	SharedCache<Vector<float3>> poly_normals_cache = {};
	SharedCache<Vector<float3>> corner_normals_cache = {};

	/**
	 * Cache of offsets for vert to face/corner maps. The same offsets array is used to group
	 * indices for both the vertex to face and vertex to corner maps.
	 */
	SharedCache<Array<int>> vert_to_face_offset_cache = {};
	/** Cache of indices for vert to face map. */
	SharedCache<Array<int>> vert_to_face_map_cache = {};
	
	/**
     * Data used to efficiently draw the mesh in the viewport, especially useful when 
     * the same mesh is used in many objects or instances. See `draw_cache_impl_mesh.c`.
	 * 
	 * \type MeshBatchCache
	 */
	void *draw_cache;
};

}  // namespace rose::kernel

#endif // KER_MESH_TYPES_HH