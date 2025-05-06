#ifndef RM_CORE_H
#define RM_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/** #RMesh element creation flags. */
enum {
	RM_CREATE_NOP = 0,
	/**
	 * This flag means that no duplicates are allowed and is allowed only for faces and edges.
	 */
	RM_CREATE_NO_DOUBLE = (1 << 0),
	/**
	 * Skip custom-data, use if we immediately write custom-data into the element so this skips
	 * copying from 'example' arguments or setting defaults, speeds up conversion when data is
	 * converted all at once.
	 */
	RM_CREATE_SKIP_CD = (1 << 1),
};

/**
 * Create a new vertex in the specified \a mesh with the specified coordinates \a co.
 * \param v_example We can use this argument to copy customdata from a source vertex to the new
 * vertex, otherwise the default customdata are copied in the new vertex unless flag
 * #RM_CREATE_SKIP_CD is specified.
 * \param flag any combination of the RM_CREATE_... flags.
 */
struct RMVert *RM_vert_create(struct RMesh *mesh, const float co[3], const struct RMVert *v_example, const int flag);

/**
 * Create a new edge in the specified \a mesh defined by the specified vertices \a v1 and \a v2.
 * \param e_example We can use this argumen to copy customdata from a source edge to the new edge,
 * otherwise the default customdata are copied in the new edge unless flag #RM_CREATE_SKIP_CD is
 * specified.
 * \param flag any combination of the RM_CREATE_... flags.
 */
struct RMEdge *RM_edge_create(struct RMesh *mesh, struct RMVert *v1, struct RMVert *v2, const struct RMEdge *e_example, const int flag);

/**
 * Create a new face in the specified \a mesh defined by the specified vertices and edges.
 * \param verts A sorted array of vertices in the winding order of the face size of \a len.
 * \param edges A sorted array of edges in the winding order of the face size of \a len.
 * \param flag any combination of the RM_CREATE_... flags.
 */
struct RMFace *RM_face_create(struct RMesh *mesh, struct RMVert **verts, struct RMEdge **edges, const int len, const struct RMFace *f_example, const int flag);

/**
 * Create a new face in the specified \a mesh defined by the specified vertices, in this method
 * edges array is not provided and therefore is extracted / created automatically.
 * \param verts A sorted array of vertices in the winding order of the face size of \a len.
 * \param create_edges wether to create edges along the vertices or use existing ones.
 * \param flag any combination of the RM_CREATE_... flags.
 */
struct RMFace *RM_face_create_verts(struct RMesh *mesh, struct RMVert **verts, const int len, const struct RMFace *f_example, const int flag, const bool create_edges);

/**
 * Duplicate a face from a mesh into another mesh (possibly the same).
 * \param mesh_dst The destination mesh the new face will be owned by.
 * \param mesh_src The source mesh the face specified by \a f is owned by.
 * \param f The face we want to duplicate.
 * \param cpy_v Wether or not we want to duplicate the vertices of that face or use existing ones.
 * \param cpy_e Wether or not we want to duplicate the edges of that face or use existing ones.
 */
struct RMFace *RM_face_copy(struct RMesh *mesh_dst, const struct RMesh *mesh_src, struct RMFace *f, bool cpy_v, bool cpy_e);

#ifdef __cplusplus
}
#endif

#endif // RM_CORE_H
