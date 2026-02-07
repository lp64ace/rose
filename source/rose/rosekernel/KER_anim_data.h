#ifndef KER_ANIM_DATA_H
#define KER_ANIM_DATA_H

#include "DNA_anim_types.h"

#include <stdbool.h>

struct AnimData;
struct Main;

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Animation Data Creation/Deletion
 * \{ */

bool id_type_can_have_animdata(const short id_type);
bool id_can_have_animdata(const struct ID *id);

/**
 * Get #AnimData from the given ID-block.
 */
struct AnimData *KER_animdata_from_id(const struct ID *id);
struct AnimData *KER_animdata_ensure_id(struct ID *id);

struct AnimData *KER_animdata_copy_ex(struct Main *main, struct AnimData *adt, int flag);

void KER_animdata_free(struct ID *id, const bool do_id_user);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Animation Data Iteration
 * \{ */

/* Define for callback looper used in BKE_fcurves_main_cb */
typedef void (*ID_FCurve_Edit_Callback)(struct ID *id, struct FCurve *fcurve, void *user_data);

/* Look over all f-curves of a given ID. */
void KER_fcurves_id_cb(struct ID *id, ID_FCurve_Edit_Callback func, void *user_data);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_ANIM_DATA_H