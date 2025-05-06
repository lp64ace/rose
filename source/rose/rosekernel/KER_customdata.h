#ifndef KER_CUSOTMDATA_H
#define KER_CUSOTMDATA_H

#include "DNA_customdata_types.h"

#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct CustomData;
struct CustomData_MeshMasks;
struct ID;

enum eCustomDataType;

typedef uint64_t eCustomDataMask;

/* These names are used as prefixes for UV layer names to find the associated boolean
 * layers. They should never be longer than 2 chars, as MAX_CUSTOMDATA_LAYER_NAME
 * has 4 extra bytes above what can be used for the base layer name, and these
 * prefixes are placed between 2 '.'s at the start of the layer name.
 * For example The uv vert selection layer of a layer named 'UVMap.001'
 * will be called '.vs.UVMap.001' . */
#define UV_VERTSEL_NAME "vs"
#define UV_EDGESEL_NAME "es"
#define UV_PINNED_NAME "pn"

/* A data type large enough to hold 1 element from any custom-data layer type. */
typedef struct {
	unsigned char data[64];
} CDBlockBytes;

extern const CustomData_MeshMasks CD_MASK_MESH;
extern const CustomData_MeshMasks CD_MASK_EVERYTHING;

typedef enum eCDAllocType {
	/**
	 * Allocate and set to default, which is usually just zeroed memory.
	 */
	CD_SET_DEFAULT = 2,
	/**
	 * Default construct new layer values. Does nothing for trivial types. This should be used
	 * if all layer values will be set by the called after creating the layer.
	 */
	CD_CONSTRUCT = 5,
} eCDAllocType;

#define CD_TYPE_AS_MASK(_type) (eCustomDataMask)((eCustomDataMask)1 << (eCustomDataMask)(_type))

typedef void (*cd_interp)(const void **sources, const float *weights, const float *sub_weights, int count, void *dest);
typedef void (*cd_copy)(const void *source, void *dest, int count);
typedef bool (*cd_validate)(void *item, uint totitems, bool do_fixes);

/* -------------------------------------------------------------------- */
/** \name Mesh Mask Utilities
 * \{ */

/**
 * Update mask_dst with layers defined in mask_src (equivalent to a bit-wise OR).
 */
void CustomData_MeshMasks_update(struct CustomData_MeshMasks *mask_dst, const struct CustomData_MeshMasks *mask_src);
/**
 * Return True if all layers set in \a mask_required are also set in \a mask_ref
 */
bool CustomData_MeshMasks_are_matching(const struct CustomData_MeshMasks *mask_ref, const struct CustomData_MeshMasks *mask_required);

/** \} */

/**
 * Checks if the layer at physical offset \a layer_n (in data->layers) support math
 * the below operations.
 */
bool CustomData_layer_has_math(const struct CustomData *data, int layer_n);
bool CustomData_layer_has_interp(const struct CustomData *data, int layer_n);

/**
 * Checks if any of the custom-data layers has math.
 */
bool CustomData_has_math(const struct CustomData *data);
bool CustomData_has_interp(const struct CustomData *data);
/**
 * A non mesh version would have to check `layer->data`.
 */
bool CustomData_rmesh_has_free(const struct CustomData *data);

/* Not really a public function but readfile.c needs it */
void CustomData_update_typemap(struct CustomData *data);

/**
 * Copies the "value" (e.g. `mloopuv` UV or `mloopcol` colors) from one block to
 * another, while not overwriting anything else (e.g. flags).  probably only
 * implemented for `mloopuv/mloopcol`, for now.
 */
void CustomData_data_copy_value(eCustomDataType type, const void *source, void *dest);
void CustomData_data_set_default_value(eCustomDataType type, void *elem);

/**
 * Mixes the "value" (e.g. `mloopuv` UV or `mloopcol` colors) from one block into
 * another, while not overwriting anything else (e.g. flags).
 */
void CustomData_data_mix_value(eCustomDataType type, const void *source, void *dest, int mixmode, float mixfactor);

/**
 * Compares if data1 is equal to data2.  type is a valid #CustomData type
 * enum (e.g. #CD_PROP_FLOAT). the layer type's equal function is used to compare
 * the data, if it exists, otherwise #memcmp is used.
 */
bool CustomData_data_equals(eCustomDataType type, const void *data1, const void *data2);
void CustomData_data_initminmax(eCustomDataType type, void *min, void *max);
void CustomData_data_dominmax(eCustomDataType type, const void *data, void *min, void *max);
void CustomData_data_multiply(eCustomDataType type, void *data, float fac);
void CustomData_data_add(eCustomDataType type, void *data1, const void *data2);

/**
 * Initializes a CustomData object with the same layer setup as source. `mask` is a bit-field where
 * `(mask & (1 << (layer type)))` indicates if a layer should be copied or not. Data layers using
 * implicit-sharing will not actually be copied but will be shared between source and destination.
 */
void CustomData_copy(const struct CustomData *source, struct CustomData *dest, eCustomDataMask mask, int totelem);
/**
 * Initializes a CustomData object with the same layers as source. The data is not copied from the
 * source. Instead, the new layers are initialized using the given `alloctype`.
 */
void CustomData_copy_layout(const struct CustomData *source, struct CustomData *dest, eCustomDataMask mask, eCDAllocType alloctype, int totelem);

/* Not really a public function but readfile.c needs it */
void CustomData_update_typemap(struct CustomData *data);

/**
 * Copies all layers from source to destination that don't exist there yet.
 */
bool CustomData_merge(const struct CustomData *source, struct CustomData *dest, eCustomDataMask mask, int totelem);
/**
 * Copies all layers from source to destination that don't exist there yet. The layer data is not
 * copied. Instead the newly created layers are initialized using the given `alloctype`.
 */
bool CustomData_merge_layout(const struct CustomData *source, struct CustomData *dest, eCustomDataMask mask, eCDAllocType alloctype, int totelem);

/**
 * Reallocate custom data to a new element count. If the new size is larger, the new values use
 * the #CD_CONSTRUCT behavior, so trivial types must be initialized by the caller. After being
 * resized, the #CustomData does not contain any referenced layers.
 */
void CustomData_realloc(struct CustomData *data, int old_size, int new_size);

/**
 * NULL's all members and resets the #CustomData.typemap.
 */
void CustomData_reset(struct CustomData *data);

/**
 * Frees data associated with a CustomData object (doesn't free the object itself, though).
 */
void CustomData_free(struct CustomData *data, int totelem);

/**
 * Same as above, but only frees layers which matches the given mask.
 */
void CustomData_free_typemask(struct CustomData *data, int totelem, eCustomDataMask mask);

/**
 * Frees all layers with #CD_FLAG_TEMPORARY.
 */
void CustomData_free_temporary(struct CustomData *data, int totelem);

/**
 * Adds a layer of the given type to the #CustomData object. The new layer is initialized based on
 * the given alloctype.
 * \return The layer data.
 */
void *CustomData_add_layer(struct CustomData *data, eCustomDataType type, eCDAllocType alloctype, int totelem);

/**
 * Adds a layer of the given type to the #CustomData object. The new layer takes ownership of the
 * passed in `layer_data`. If a #ImplicitSharingInfoHandle is passed in, its user count is
 * increased.
 */
const void *CustomData_add_layer_with_data(struct CustomData *data, eCustomDataType type, void *layer_data, int totelem, const ImplicitSharingInfoHandle *sharing_info);

/**
 * Same as above but accepts a name.
 */
void *CustomData_add_layer_named(struct CustomData *data, eCustomDataType type, eCDAllocType alloctype, int totelem, const char *name);

const void *CustomData_add_layer_named_with_data(struct CustomData *data, eCustomDataType type, void *layer_data, int totelem, const char *name, const ImplicitSharingInfoHandle *sharing_info);

bool CustomData_has_layer_named(const struct CustomData *data, eCustomDataType type, const char *name);

/**
 * Frees the active or first data layer with the give type.
 * returns 1 on success, 0 if no layer with the given type is found
 *
 * In edit-mode, use #EDBM_data_layer_free instead of this function.
 */
bool CustomData_free_layer(struct CustomData *data, eCustomDataType type, int totelem, int index);
bool CustomData_free_layer_named(struct CustomData *data, const char *name, const int totelem);

/**
 * Frees the layer index with the give type.
 * returns 1 on success, 0 if no layer with the given type is found.
 *
 * In edit-mode, use #EDBM_data_layer_free instead of this function.
 */
bool CustomData_free_layer_active(struct CustomData *data, eCustomDataType type, int totelem);

/**
 * Same as above, but free all layers with type.
 */
void CustomData_free_layers(struct CustomData *data, eCustomDataType type, int totelem);

/**
 * Returns true if a layer with the specified type exists.
 */
bool CustomData_has_layer(const struct CustomData *data, eCustomDataType type);

/**
 * Returns the number of layers with this type.
 */
int CustomData_number_of_layers(const struct CustomData *data, eCustomDataType type);
int CustomData_number_of_layers_typemask(const struct CustomData *data, eCustomDataMask mask);

/**
 * Set the #CD_FLAG_NOCOPY flag in custom data layers where the mask is
 * zero for the layer type, so only layer types specified by the mask will be copied
 */
void CustomData_set_only_copy(const struct CustomData *data, eCustomDataMask mask);

/**
 * Copies data from one CustomData object to another
 * objects need not be compatible, each source layer is copied to the
 * first dest layer of correct type (if there is none, the layer is skipped).
 */
void CustomData_copy_data(const struct CustomData *source, struct CustomData *dest, int source_index, int dest_index, int count);
void CustomData_copy_data_layer(const struct CustomData *source, struct CustomData *dest, int src_layer_index, int dst_layer_index, int src_index, int dst_index, int count);
void CustomData_copy_data_named(const struct CustomData *source, struct CustomData *dest, int source_index, int dest_index, int count);
void CustomData_copy_elements(eCustomDataType type, void *src_data_ofs, void *dst_data_ofs, int count);

void CustomData_rmesh_copy_data(const struct CustomData *source, struct CustomData *dest, void *src_block, void **dest_block);
void CustomData_rmesh_copy_data_exclude_by_type(const struct CustomData *source, struct CustomData *dest, void *src_block, void **dest_block, eCustomDataMask mask_exclude);

/**
 * Copies data of a single layer of a given type.
 */
void CustomData_copy_layer_type_data(const struct CustomData *source, struct CustomData *destination, eCustomDataType type, int source_index, int destination_index, int count);

/**
 * Frees data in a #CustomData object.
 */
void CustomData_free_elem(struct CustomData *data, int index, int count);

/**
 * Interpolate given custom data source items into a single destination one.
 *
 * \param src_indices: Indices of every source items to interpolate into the destination one.
 * \param weights: The weight to apply to each source value individually. If NULL, they will be
 * averaged.
 * \param sub_weights: The weights of sub-items, only used to affect each corners of a
 * tessellated face data (should always be and array of four values).
 * \param count: The number of source items to interpolate.
 * \param dest_index: Index of the destination item, in which to put the result of the
 * interpolation.
 */
void CustomData_interp(const struct CustomData *source, struct CustomData *dest, const int *src_indices, const float *weights, const float *sub_weights, int count, int dest_index);

/**
 * Swap data inside each item, for all layers.
 * This only applies to item types that may store several sub-item data
 * (e.g. corner data [UVs, VCol, ...] of tessellated faces).
 *
 * \param corner_indices: A mapping 'new_index -> old_index' of sub-item data.
 */
void CustomData_swap_corners(struct CustomData *data, int index, const int *corner_indices);

/**
 * Swap two items of given custom data, in all available layers.
 */
void CustomData_swap(struct CustomData *data, int index_a, int index_b);

/**
 * Gets from the layer at physical index `n`,
 * \note doesn't check type.
 */
void *CustomData_mesh_get_layer_n(const struct CustomData *data, void *block, int n);

bool CustomData_set_layer_name(struct CustomData *data, eCustomDataType type, int n, const char *name);
const char *CustomData_get_layer_name(const struct CustomData *data, eCustomDataType type, int n);

/**
 * Retrieve the data array of the active layer of the given \a type, if it exists. Return null
 * otherwise.
 */
const void *CustomData_get_layer(const struct CustomData *data, eCustomDataType type);
void *CustomData_get_layer_for_write(struct CustomData *data, eCustomDataType type, int totelem);

/**
 * Retrieve the data array of the \a nth layer of the given \a type, if it exists. Return null
 * otherwise.
 */
const void *CustomData_get_layer_n(const struct CustomData *data, eCustomDataType type, int n);

/**
 * Retrieve the data array of the layer with the given \a name and \a type, if it exists. Return
 * null otherwise.
 */
const void *CustomData_get_layer_named(const struct CustomData *data, const eCustomDataType type, const char *name);
void *CustomData_get_layer_named_for_write(struct CustomData *data, const eCustomDataType type, const char *name, const int totelem);

int CustomData_get_offset(const struct CustomData *data, eCustomDataType type);
int CustomData_get_offset_named(const struct CustomData *data, eCustomDataType type, const char *name);
int CustomData_get_n_offset(const struct CustomData *data, eCustomDataType type, int n);

int CustomData_get_layer_index(const struct CustomData *data, eCustomDataType type);
int CustomData_get_layer_index_n(const struct CustomData *data, eCustomDataType type, int n);
int CustomData_get_named_layer_index(const struct CustomData *data, eCustomDataType type, const char *name);
int CustomData_get_named_layer_index_notype(const struct CustomData *data, const char *name);
int CustomData_get_active_layer_index(const struct CustomData *data, eCustomDataType type);
int CustomData_get_render_layer_index(const struct CustomData *data, eCustomDataType type);
int CustomData_get_clone_layer_index(const struct CustomData *data, eCustomDataType type);
int CustomData_get_stencil_layer_index(const struct CustomData *data, eCustomDataType type);
int CustomData_get_named_layer(const struct CustomData *data, eCustomDataType type, const char *name);
int CustomData_get_active_layer(const struct CustomData *data, eCustomDataType type);
int CustomData_get_render_layer(const struct CustomData *data, eCustomDataType type);
int CustomData_get_clone_layer(const struct CustomData *data, eCustomDataType type);
int CustomData_get_stencil_layer(const struct CustomData *data, eCustomDataType type);

/**
 * Returns name of the active layer of the given type or NULL
 * if no such active layer is defined.
 */
const char *CustomData_get_active_layer_name(const struct CustomData *data, eCustomDataType type);

/**
 * Returns name of the default layer of the given type or NULL
 * if no such active layer is defined.
 */
const char *CustomData_get_render_layer_name(const struct CustomData *data, eCustomDataType type);

/**
 * Sets the nth layer of type as active.
 */
void CustomData_set_layer_active(struct CustomData *data, eCustomDataType type, int n);
void CustomData_set_layer_render(struct CustomData *data, eCustomDataType type, int n);
void CustomData_set_layer_clone(struct CustomData *data, eCustomDataType type, int n);
void CustomData_set_layer_stencil(struct CustomData *data, eCustomDataType type, int n);

/**
 * For using with an index from #CustomData_get_active_layer_index and
 * #CustomData_get_render_layer_index.
 */
void CustomData_set_layer_active_index(struct CustomData *data, eCustomDataType type, int n);
void CustomData_set_layer_render_index(struct CustomData *data, eCustomDataType type, int n);
void CustomData_set_layer_clone_index(struct CustomData *data, eCustomDataType type, int n);
void CustomData_set_layer_stencil_index(struct CustomData *data, eCustomDataType type, int n);

/**
 * Adds flag to the layer flags.
 */
void CustomData_set_layer_flag(struct CustomData *data, eCustomDataType type, int flag);
void CustomData_clear_layer_flag(struct CustomData *data, eCustomDataType type, int flag);

void CustomData_rmesh_set_default(struct CustomData *data, void **block);
void CustomData_rmesh_free_block(struct CustomData *data, void **block);
void CustomData_rmesh_alloc_block(struct CustomData *data, void **block);

/**
 * Same as #CustomData_mesh_free_block but zero the memory rather than freeing.
 */
void CustomData_rmesh_free_block_data(struct CustomData *data, void *block);
void CustomData_rmesh_free_block_data_exclude_by_type(struct CustomData *data, void *block, eCustomDataMask mask_exclude);

int CustomData_sizeof(eCustomDataType type);

/**
 * Get the name of a layer type.
 */
const char *CustomData_layertype_name(eCustomDataType type);
/**
 * Can only ever be one of these.
 */
bool CustomData_layertype_is_singleton(eCustomDataType type);
/**
 * Has dynamically allocated members.
 * This is useful to know if operations such as #memcmp are
 * valid when comparing data from two layers.
 */
bool CustomData_layertype_is_dynamic(eCustomDataType type);
/**
 * \return Maximum number of layers of given \a type, -1 means 'no limit'.
 */
int CustomData_layertype_layers_max(eCustomDataType type);

/**
 * Make sure the name of layer at index is unique.
 */
void CustomData_set_layer_unique_name(struct CustomData *data, int index);

void CustomData_validate_layer_name(const struct CustomData *data, eCustomDataType type, const char *name, char *outname);

/**
 * Validate and fix data of \a layer,
 * if possible (needs relevant callback in layer's type to be defined).
 *
 * \return True if some errors were found.
 */
bool CustomData_layer_validate(struct CustomDataLayer *layer, uint totitems, bool do_fixes);

/**
 * How to filter out some elements (to leave untouched).
 * Note those options are highly dependent on type of transferred data! */
enum {
	CDT_MIX_NOMIX = -1, /* Special case, only used because we abuse 'copy' CD callback. */
	CDT_MIX_TRANSFER = 0,
	CDT_MIX_REPLACE_ABOVE_THRESHOLD = 1,
	CDT_MIX_REPLACE_BELOW_THRESHOLD = 2,
	CDT_MIX_MIX = 16,
	CDT_MIX_ADD = 17,
	CDT_MIX_SUB = 18,
	CDT_MIX_MUL = 19,
	/* Etc. */
};

size_t CustomData_get_elem_size(const struct CustomDataLayer *layer);

#ifdef __cplusplus
}
#endif

#endif	// KER_CUSOTMDATA_H
