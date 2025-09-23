#ifndef EXTRACT_MESH_H
#define EXTRACT_MESH_H

#include "DRW_cache.h"

#include "KER_mesh.h"
#include "KER_object.h"

struct Mesh;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Vertex
 * \{ */

void extract_positions(const struct Mesh *mesh, struct GPUVertBuf *vbo);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Trianglulation
 * \{ */

void extract_triangles(const struct Mesh *mesh, struct GPUIndexBuf *ibo);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// EXTRACT_MESH_H
