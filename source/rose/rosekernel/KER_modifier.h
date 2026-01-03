#ifndef KER_MODIFIER_H
#define KER_MODIFIER_H

#include "DNA_modifier_types.h"

#include "KER_customdata.h"

#include "LIB_utildefines.h"

struct ID;
struct Mesh;
struct ModifierData;
struct ModifierEvalContext;
struct Object;
struct Scene;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Modifier Type Info
 * \{ */

typedef void (*IDWalkFunc)(void *userdata, struct Object *object, struct ID **idpointer, int flag);

typedef enum eModifierInfoType {
	/**
	 * Modifier only does deformation, implies that modifier type should have a 
	 * valid deform_verts function. OnlyDeform style modifiers implicitly accept either mesh 
	 * input but should still declare flags appropriately.
	 */
	OnlyDeform,
} eModifierInfoType;

typedef enum eModifierInfoFlag {
	eModifierTypeFlag_AcceptsMesh = (1 << 0),
} eModifierInfoFlag;

ENUM_OPERATORS(eModifierInfoFlag, eModifierTypeFlag_AcceptsMesh)

typedef struct ModifierTypeInfo {
	/**
	 * A unique identifier for this modifier, used to generate the panel id type name.
	 */
	char idname[64];
	char name[64];

	/**
	 * The DNA struct name for the modifier data type, 
	 * used to write the DNA data out.
	 */
	char dnastruct[64];

	size_t size;

	eModifierInfoType type;
	eModifierInfoFlag flag;

	/********************* Non-optional functions *********************/

	/**
	 * Copy instance data for this modifier type. Should copy all user level 
	 * setting to the target modifier.
	 * 
	 * \param flag: Copying options (see KER_lib_id.h's LIB_ID_COPY_... flags for mode).
	 */
	void (*copy_data)(const struct ModifierData *src, struct ModifierData *dst, int flag);

	/********************* Deform modifier functions *********************/

	/**
	 * Apply a deformation to the positions in the \a positions array. If the \a mesh argument is
	 * non-null, if will contain proper (not wrapped) mesh data. The \a positions array may or may
	 * not be the same as the mesh's position attribute.
	 */
	void (*deform_verts)(struct ModifierData *md, const struct ModifierEvalContext *ctx, struct Mesh *mesh, float (*positions)[3], size_t length);

	/********************* Non-Deform modifier functions *********************/



	/********************* Optional functions *********************/

	/**
	 * Initialize new instance data for this modifier type, this function
	 * should set modifier variables to their default values.
	 *
	 * This function is optional.
	 */
	void (*init_data)(struct ModifierData *md);

	/**
	 * Should add to passed \a r_cddata_masks the data types that this
	 * modifier needs. If (mask & (1 << (layer type))) != 0, this modifier
	 * needs that custom data layer. It can change required layers
	 * depending on the modifier's settings.
	 *
	 * Note that this means extra data (e.g. vertex groups) - it is assumed
	 * that all modifiers need mesh data and deform modifiers need vertex
	 * coordinates.
	 *
	 * If this function is not present, it is assumed that no extra data is needed.
	 *
	 * This function is optional.
	 */
	void (*required_data_mask)(struct ModifierData *md, CustomData_MeshMasks *r_cddata_masks);

	/**
	 * Free internal modifier data variables, this function should
	 * not free the md variable itself.
	 *
	 * This function is responsible for freeing the runtime data as well.
	 *
	 * This function is optional.
	 */
	void (*free_data)(struct ModifierData *md);

	/**
	 * Should return true if the modifier needs to be recalculated on time
	 * changes.
	 *
	 * This function is optional (assumes false if not present).
	 */
	bool (*depends_on_time)(struct Scene *scene, struct ModifierData *md);

	/**
	 * Returns true when a deform modifier uses mesh normals as input. This callback is only required
	 * for deform modifiers that support deforming positions with an edit mesh (when #deform_verts_EM
	 * is implemented).
	 */
	bool (*depends_on_normals)(struct ModifierData *md);

	/**
	 * Should call the given walk function with a pointer to each ID
	 * pointer (i.e. each data-block pointer) that the modifier data
	 * stores. This is used for linking on file load and for
	 * unlinking data-blocks or forwarding data-block references.
	 *
	 * This function is optional.
	 */
	void (*foreach_ID_link)(struct ModifierData *md, struct Object *ob, IDWalkFunc walk, void *user_data);

	/**
	 * Free given run-time data.
	 *
	 * This data is coming from a modifier of the corresponding type, but actual
	 * modifier data is not known here.
	 *
	 * Notes:
	 *  - The data itself is to be de-allocated as well.
	 *  - This callback is allowed to receive NULL pointer as a data, so it's
	 *    more like "ensure the data is freed".
	 */
	void (*free_runtime_data)(void *runtime_data);

} ModifierTypeInfo;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Modifier Type Info Methods
 * \{ */

void KER_modifier_init(void);

const struct ModifierTypeInfo *KER_modifier_get_info(eModifierType type);

/** \} */

/* -------------------------------------------------------------------- */
/** \name ModifierData Methods
 * \{ */

struct ModifierData *KER_modifier_new(eModifierType type);
struct ModifierData *KER_modifier_copy_ex(const struct ModifierData *md, int flag);

void KER_modifier_free_ex(struct ModifierData *md, int flag);
void KER_modifier_free(struct ModifierData *md);

void KER_modifier_remove_from_list(struct Object *object, struct ModifierData *md);

typedef struct ModifierEvalContext {
	struct Scene *scene;
	struct Object *object;
} ModifierEvalContext;

/**
 * \return False if the modifier did not support deforming the positions.
 */
bool KER_modifier_deform_verts(struct ModifierData *md, const struct ModifierEvalContext *ctx, struct Mesh *mesh, float (*positions)[3], size_t length);

/** \} */

/* -------------------------------------------------------------------- */
/** \name ModifierData for each Query
 * \{ */

void KER_modifiers_foreach_ID_link(struct Object *object, IDWalkFunc walk, void *user_data);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_MODIFIER_H
