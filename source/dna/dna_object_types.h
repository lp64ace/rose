#pragma once

#include "dna_customdata_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct BoundBox {
		float Vec [ 8 ][ 3 ];
		int Flag;
	} BoundBox;

	enum {
		/* BOUNDBOX_DISABLED = (1 << 0), */
		BOUNDBOX_DIRTY = ( 1 << 1 ) ,
	};

	struct CustomData_MeshMasks;

	typedef struct Object_Runtime {
		/** The custom data layer mask that was last used
		* to calculate data_eval and mesh_deform_eval. */
		CustomData_MeshMasks LastDataMask;

		
	} Object_Runtime;

	typedef struct Object {
		float Location [ 3 ];
		float Scale [ 3 ];
		float Rot [ 3 ];
		float Quat [ 4 ];
		float RotAxis [ 3 ];
		float RotAngle;
		float ParentInv [ 4 ][ 4 ];
		float Matrix [ 4 ][ 4 ];

		// Object color (in most cases the material color is used for drawing).
		float Color [ 4 ];

		// For restricting view, select, render etc. accessible in outliner.
		short VisibilityFlag;
	} Object;

#ifdef __cplusplus
}
#endif
