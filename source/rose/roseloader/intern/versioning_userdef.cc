#include "DNA_userdef_types.h"

#include "RLO_read_write.h"

ROSE_STATIC void do_versions_theme(const UserDef *userdef, Theme *theme) {
}

void rlo_do_versions_userdef(UserDef *userdef) {
	LISTBASE_FOREACH(Theme *, theme, &userdef->themes) {
		do_versions_theme(userdef, theme);
	}
}
