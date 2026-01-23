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

/**
 * Instead of filling the VBO with the positions of each vertex, 
 * the VBO receives line segments representing the normals at each face.
 */
void extract_normal_lines(const Mesh *mesh, struct GPUVertBuf *vbo);

void extract_weights(const Object *obarmature, const Object *obtarget, const Mesh *mesh, struct GPUVertBuf *vbo);
void extract_matrices(const Object *obarmature, const Object *obtarget, const Mesh *mesh, struct GPUUniformBuf *ubo);

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
