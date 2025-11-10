#ifndef EXTRACT_MESH_H
#define EXTRACT_MESH_H

#include "DRW_cache.h"

#include "KER_mesh.h"
#include "KER_object.h"

#include <stdbool.h>

struct GPUIndexBuf;
struct GPUUniformBuf;
struct GPUVertBuf;

struct Mesh;
struct Object;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Vertex
 * \{ */

void extract_positions(const struct Mesh *mesh, struct GPUVertBuf *vbo);
void extract_normals(const Mesh *mesh, struct GPUVertBuf *vbo, bool use_hq);

/** Returns the uniform buffer that will be used for the armature matrices! */
struct GPUUniformBuf *extract_weights(const Object *obarmature, const Object *obtarget, const Mesh *mesh, struct GPUVertBuf *vbo);

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
