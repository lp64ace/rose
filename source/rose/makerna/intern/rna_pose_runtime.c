#include "DNA_action_types.h"

#include "KER_action.h"

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_prototypes.h"

#include "rna_internal.h"
#include "rna_internal_types.h"

/* -------------------------------------------------------------------- */
/** \name Exposed Functions
 * \{ */

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
