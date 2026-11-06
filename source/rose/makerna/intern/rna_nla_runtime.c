#include "DNA_action_types.h"

#include "KER_action.h"
#include "KER_anim_data.h"

#include "LIB_string.h"
#include "LIB_string_utils.h"

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

char *rna_NlaStrip_path(const PointerRNA *ptr) {
	NlaStrip *data = (NlaStrip *)ptr->data;
	AnimData *adt = KER_animdata_from_id(ptr->owner);

	if (adt) {
		LISTBASE_FOREACH (NlaTrack *, track, &adt->nla_tracks) {
			LISTBASE_FOREACH (NlaStrip *, strip, &track->strips) {
				if (data == strip) {
					/* XXX but if we animate like this, the control will never work... */
					char name_esc_track[sizeof(track->name) * 2];
					char name_esc_strip[sizeof(strip->name) * 2];

					LIB_str_escape(name_esc_track, sizeof(name_esc_track), track->name);
					LIB_str_escape(name_esc_strip, sizeof(name_esc_strip), strip->name);
					return LIB_strformat_allocN("animation_data.nla_tracks[\"%s\"].strips[\"%s\"]", name_esc_track, name_esc_strip);
				}
			}
		}
	}
}

/** \} */
