#pragma once

#include "lib/lib_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

	#define MAX_CUSTOMDATA_LAYER_NAME 64

	struct AnonymousAttributeID;

	typedef struct CustomDataLayer {
		// Type of data in layer.
		int Type;
		// Offset of layer in block.
		int Offset;
		// General purpose flag.
		int Flag;
		// Unique id reference
		int UniqueId;
		// Layer name, MAX_CUSTOMDATA_LAYER_NAME.
		char Name [ 64 ];
		// Number of the active layer of this type.
		int Active;
		// Layer data.
		void *Data;
		/** Run-time identifier for this layer. If no one has a strong reference to this id anymore,
		* the layer can be removed. The custom data layer only has a weak reference to the id, because
		* otherwise there will always be a strong reference and the attribute can't be removed
		* automatically. */
		const struct AnonymousAttributeID *AnonymousId;
	} CustomDataLayer;

	typedef struct CustomData {
		// CustomDataLayers, ordered by type.
		CustomDataLayer *Layers;
		// Runtime Only! - Maps types to indices of first layer of that type.
		int TypeMap [ 16 ];
		// Number of layers, size of layers array.
		int TotLayer , MaxLayer;
		// Total size of all data layers.
		int TotSize;
		// Memory pool for allocation of blocks.
		struct LIB_mempool *Pool;
	} CustomData;

	// CustomDataLayer::Type
	enum eCustomDataType {
		/* Used by GLSL attributes in the cases when we need a delayed CD type
		* assignment (in the cases when we don't know in advance which layer
		* we are addressing).
		*/
		CD_AUTO_FROM_NAME = -1 ,

		CD_MVERT ,
		CD_NORMAL ,
		CD_PROP_FLOAT ,
		CD_PROP_INT32 ,
		CD_PROP_STRING ,
		CD_MLOOPUV ,
		CD_PROP_BYTE_COLOR ,

		CD_SHAPE_KEYINDEX ,

		CD_PROP_INT8 ,

		CD_PROP_COLOR ,
		CD_PROP_FLOAT3 ,
		CD_PROP_FLOAT2 ,
		CD_PROP_BOOL ,

		CD_NUMTYPES ,
	};

	#define CD_MASK_MVERT (1ULL << CD_MVERT)

	#define CD_MASK_NORMAL (1ULL << CD_NORMAL)
	#define CD_MASK_PROP_FLOAT (1ULL << CD_PROP_FLOAT)
	#define CD_MASK_PROP_INT32 (1ULL << CD_PROP_INT32)
	#define CD_MASK_PROP_STRING (1ULL << CD_PROP_STRING)

	#define CD_MASK_MLOOPUV (1ULL << CD_MLOOPUV)
	#define CD_MASK_PROP_BYTE_COLOR (1ULL << CD_PROP_BYTE_COLOR)

	#define CD_MASK_SHAPE_KEYINDEX (1ULL << CD_SHAPE_KEYINDEX)

	#define CD_MASK_PROP_COLOR (1ULL << CD_PROP_COLOR)
	#define CD_MASK_PROP_FLOAT3 (1ULL << CD_PROP_FLOAT3)
	#define CD_MASK_PROP_FLOAT2 (1ULL << CD_PROP_FLOAT2)
	#define CD_MASK_PROP_BOOL (1ULL << CD_PROP_BOOL)
	#define CD_MASK_PROP_INT8 (1ULL << CD_PROP_INT8)

	ROSE_STATIC_ASSERT ( ARRAY_SIZE ( CustomData::TypeMap ) >= CD_NUMTYPES , "Too many CustomData types" );

	typedef struct CustomData_MeshMasks {
		uint64_t VertMask;
		uint64_t EdgeMask;
		uint64_t FaceMask;
		uint64_t PolyMask;
		uint64_t LoopMask;
	} CustomData_MeshMasks;

	// CustomDataLayer::Flag
	enum {
		/* Indicates layer should not be copied by CustomData_from_template or CustomData_copy_data */
		CD_FLAG_NOCOPY = ( 1 << 0 ) ,
		/* Indicates layer should not be freed (for layers backed by external data) */
		CD_FLAG_NOFREE = ( 1 << 1 ) ,
		/* Indicates the layer is only temporary, also implies no copy */
		CD_FLAG_TEMPORARY = ( ( 1 << 2 ) | CD_FLAG_NOCOPY ) ,
		/* Indicates the layer is stored in an external file */
		CD_FLAG_EXTERNAL = ( 1 << 3 ) ,
		/* Indicates external data is read into memory */
		CD_FLAG_IN_MEMORY = ( 1 << 4 ) ,
		CD_FLAG_COLOR_ACTIVE = ( 1 << 5 ) ,
		CD_FLAG_COLOR_RENDER = ( 1 << 6 )
	};

	#define MAX_MTFACE 8

#ifdef __cplusplus
}
#endif
