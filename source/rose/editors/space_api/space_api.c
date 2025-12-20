#include "DNA_windowmanager_types.h"

#include "ED_screen.h"
#include "ED_space_api.h"

#include "KER_screen.h"

void ED_spacetypes_init() {
	ED_spacetype_statusbar();
	ED_spacetype_topbar();
	ED_spacetype_view3d();
	ED_spacetype_properties();

	ED_operatortypes_screen();

	ListBase *lb = KER_spacetype_list();

	/* Register types for operators. */
	LISTBASE_FOREACH(SpaceType *, st, lb) {
		if (st->operatortypes) {
			st->operatortypes();
		}
	}
}

void ED_spacetypes_exit() {
	/** Free all the spacetypes that were registered! */
	KER_spacetype_free();
}

void ED_spacetypes_keymap(wmKeyConfig *keyconf) {
	ED_keymap_screen(keyconf);

	ListBase *lb = KER_spacetype_list();

	/* Register types for operators. */
	LISTBASE_FOREACH(SpaceType *, st, lb) {
		if (st->keymap) {
			st->keymap(keyconf);
		}
		LISTBASE_FOREACH(ARegionType *, rt, &st->regiontypes) {
			if (rt->keymap) {
				rt->keymap(keyconf);
			}
		}
	}
}
