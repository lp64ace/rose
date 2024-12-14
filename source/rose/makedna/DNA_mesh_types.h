#ifndef DNA_MESH_TYPES_H
#define DNA_MESH_TYPES_H

#include "DNA_ID.h"

#include "DNA_customdata_types.h"
#include "DNA_meshdata_types.h"

#include <stdint.h>

#ifdef __cplusplus
namespace rose::kernel {
struct MeshRuntime;
}  // namespace rose::kernel
using MeshRuntime = rose::kernel::MeshRuntime;
#else
typedef struct MeshRuntime MeshRuntime;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Mesh {
	ID id;
	
	/** The number of vertices in the mesh, and the size of #vdata. */
	int totvert;
	/** The number of edges in the mesh, and the size of #edata. */
	int totedge;
	/** The number of polygons/faces in the mesh, and the size of #pdata. */
	int totpoly;
	/** The number of face corners in the mesh, and the size of #ldata. */
	int totloop;
	
	CustomData vdata;
	CustomData edata;
	CustomData pdata;
	CustomData ldata;
	
	/**
	 * Polygon topology storage of the offset of each poly's section of the #mloop poly corner array.
	 */
	int *poly_offset_indices;
	
	/** Mostly flags used when editing or displaying the mesh. */
	int flag;
	
	/** Storage of faces (only triangles or quads). */
	CustomData fdata;
	
	int totface;
	
	/**
	 * Data that isn't saved in files, including caches of derived data, temporary data to improve
	 * the editing experience, etc. The struct is created when reading files and can be accessed
	 * without null checks, with the exception of some temporary meshes which should allocate and
	 * free the data if they are passed to functions that expect run-time data.
	 */
	MeshRuntime *runtime;
} Mesh;

#define MESH_MAX_VERTS 16777216

#ifdef __cplusplus
}
#endif

#endif // DNA_MESH_TYPES_H
