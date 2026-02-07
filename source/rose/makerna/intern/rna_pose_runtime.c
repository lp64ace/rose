#include "DNA_action_types.h"

#include "KER_action.h"
#include "KER_object.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "DEG_depsgraph.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

struct Main;
struct Scene;

/* -------------------------------------------------------------------- */
/** \name Exposed Functions
 * \{ */

void rna_Pose_update(struct Main *main, struct Scene *scene, PointerRNA *ptr) {
	Object *ob = (Object *)ptr->owner;

	DEG_id_tag_update(&ob->id, ID_RECALC_GEOMETRY);
}

bool rna_PoseBones_lookup_string(PointerRNA *ptr, const char *key, PointerRNA *r_ptr) {
	Pose *pose = (Pose *)ptr->data;
	PoseChannel *pchannel = KER_pose_channel_find_name(pose, key);
	if (pchannel) {
		rna_pointer_create_with_ancestors(ptr, &RNA_PoseBone, pchannel, r_ptr);
		return true;
	}
	return false;
}

/** \} */
