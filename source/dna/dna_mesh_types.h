#pragma once

#include "dna_customdata_types.h"
#include "dna_meshdata_types.h"

typedef struct MLoopTri {
	unsigned int Tri [ 3 ];
	unsigned int Poly;
} MLoopTri;

typedef struct MVertTri {
	unsigned int Tri [ 3 ];
} MVertTri;

typedef struct MLoopTri_Store {
	/* WARNING! swapping between array (ready-to-be-used data) and array_wip
	* (where data is actually computed) */
	struct MLoopTri *Array , *ArrayWip;

	int Len;
	int LenAlloc;
} MLoopTri_Store;

typedef struct Mesh_Runtime {
	// Cache for derived triangulation of the mesh.
	struct MLoopTri_Store LoopTris;

	/** Data used to efficiently draw the mesh in the viewport, especially useful when 
	* the same mesh is used in many objects or instances. */
	void *BatchCache;

	// Needed in case we need to lazily initialize the mesh.
	CustomData_MeshMasks CustomDataExtraMask;
} Mesh_Runtime;

typedef struct Mesh {
	
} Mesh;
