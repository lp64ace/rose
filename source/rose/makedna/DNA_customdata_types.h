#ifndef DNA_CUSTOMDATA_TYPES_H
#define DNA_CUSTOMDATA_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
namespace rose {
class ImplicitSharingInfo;
} // namespace rose
using ImplicitSharingInfoHandle = rose::ImplicitSharingInfo;
#else
typedef struct ImplicitSharingInfoHandle ImplicitSharingInfoHandle;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CUSTOMDATA_LAYER_NAME 68
#define MAX_CUSTOMDATA_LAYER_NAME_NO_PREFIX 64

typedef struct CustomDataLayer {
	/** Type of data in layer. */
	int type;
	/** In editmode, offset of layer in block. */
	int offset;
	/** General purpose flag. */
	int flag;
	/** Number of the active layer of this type. */
	int active;
	/** Number of the layer to render. */
	int active_rnd;
	/** Number of the layer to render. */
	int active_clone;
	/** Number of the layer to render. */
	int active_mask;
	/** Shape key-block unique id reference. */
	int uid;
	/** Layer name. */
	char name[MAX_CUSTOMDATA_LAYER_NAME];

	/** Layer data. */
	void *data;

	/**
	 * Run-time data that allows sharing `data` with other entities (mostly custom data layers on
	 * other geometries).
	 */
	const ImplicitSharingInfoHandle *sharing_info;
} CustomDataLayer;

/** #CustomData->type */
typedef enum eCustomDataType {
	/**
	 * Used by GLSL attributes in the cases when we need a delayed CD type
	 * assignment (in the cases when we don't know in advance which layer
	 * we are addressing).
	 */
	CD_AUTO_FROM_NAME = -1,

	CD_MDEFORMVERT = 2,

	/**
	 * Used for derived face corner normals on mesh `ldata`, since currently they are not computed
	 * lazily. Derived vertex and polygon normals are stored in #Mesh_Runtime.
	 */
	CD_NORMAL = 8,

	CD_PROP_FLOAT = 10,
	CD_PROP_INT32 = 11,
	CD_PROP_STRING = 12,
	CD_PROP_BYTE_COLOR = 17,

	CD_PROP_FLOAT4X4 = 20,

	CD_PROP_INT8 = 45,
	CD_PROP_INT32_2D = 46,
	CD_PROP_COLOR = 47,
	CD_PROP_FLOAT3 = 48,
	CD_PROP_FLOAT2 = 49,
	CD_PROP_BOOL = 50,

	CD_HAIRLENGTH = 51,

	CD_PROP_QUATERNION = 52,

	CD_NUMTYPES = 53,
} eCustomDataType;

/* Bits for eCustomDataMask */
#define CD_MASK_NORMAL (1 << CD_NORMAL)

#define CD_MASK_PROP_FLOAT (1ULL << CD_PROP_FLOAT)
#define CD_MASK_PROP_INT32 (1ULL << CD_PROP_INT32)
#define CD_MASK_PROP_STRING (1ULL << CD_PROP_STRING)
#define CD_MASK_PROP_BYTE_COLOR (1ULL << CD_PROP_BYTE_COLOR)

#define CD_MASK_PROP_COLOR (1ULL << CD_PROP_COLOR)
#define CD_MASK_PROP_FLOAT3 (1ULL << CD_PROP_FLOAT3)
#define CD_MASK_PROP_FLOAT2 (1ULL << CD_PROP_FLOAT2)
#define CD_MASK_PROP_BOOL (1ULL << CD_PROP_BOOL)
#define CD_MASK_PROP_INT8 (1ULL << CD_PROP_INT8)
#define CD_MASK_PROP_INT32_2D (1ULL << CD_PROP_INT32_2D)

#define CD_MASK_HAIRLENGTH (1ULL << CD_HAIRLENGTH)

/* All data layers. */
#define CD_MASK_ALL (~0LL)

typedef struct CustomDataExternal {
	char filepath[1024];
} CustomDataExternal;

/**
 * Structure which stores custom element data associated with mesh elements
 * (vertices, edges or faces). The custom data is organized into a series of
 * layers, each with a data type (e.g. MTFace, MDeformVert, etc.).
 */
typedef struct CustomData {
	CustomDataLayer *layers;
	/**
	 * runtime only! - maps types to indices of first layer of that type,
	 * MUST be >= CD_NUMTYPES, but we can't use a define here.
	 * Correct size is ensured in CustomData_update_typemap assert().
	 */
	int typemap[CD_NUMTYPES];
	/** Number of layers, size of layers array. */
	int totlayer, maxlayer;
	/** Total size of all data layers. */
	int totsize;

	/** Memory pool for allocation of blocks. */
	struct MemPool *pool;
	/** External file storing custom-data layers. */
	CustomDataExternal *external;
} CustomData;

/* clang-format off */

/* All generic attributes. */
#define CD_MASK_PROP_ALL \
    (CD_MASK_PROP_FLOAT | CD_MASK_PROP_FLOAT2 | CD_MASK_PROP_FLOAT3 | CD_MASK_PROP_INT32 | \
     CD_MASK_PROP_COLOR | CD_MASK_PROP_STRING | CD_MASK_PROP_BYTE_COLOR | CD_MASK_PROP_BOOL | \
     CD_MASK_PROP_INT8 | CD_MASK_PROP_INT32_2D)

/* clang-format on */

/* All color attributes */
#define CD_MASK_COLOR_ALL (CD_MASK_PROP_COLOR | CD_MASK_PROP_BYTE_COLOR)

typedef struct CustomData_MeshMasks {
	/**
	 * Do not use long long here, since long long is 128 bits in Linux 
	 * and we currently have no way to convert it to a 64 bit integer for Windows.
	 */
	uint64_t vmask;
	uint64_t emask;
	uint64_t fmask;
	uint64_t pmask;
	uint64_t lmask;
} CustomData_MeshMasks;

enum {
	/** Indicates layer should not be copied by CustomData_from_template or CustomData_copy_data. */
	CD_FLAG_NOCOPY = (1 << 0),
	CD_FLAG_UNUSED = (1 << 1),
	/** Indicates the layer is only temporary, also implies no copy. */
	CD_FLAG_TEMPORARY = ((1 << 2) | CD_FLAG_NOCOPY),
	/** Indicates the layer is stored in an external file. */
	CD_FLAG_EXTERNAL = (1 << 3),
	/** Indicates external data is read into memory. */
	CD_FLAG_IN_MEMORY = (1 << 4),
};

#ifdef __cplusplus
}
#endif

#endif // DNA_CUSTOMDATA_TYPES_H