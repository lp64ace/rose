#include "MEM_guardedalloc.h"

#include "DNA_userdef_types.h"

#include "KER_global.h"
#include "KER_main.h"
#include "KER_rose.h"
#include "KER_userdef.h"

#include <string.h>

Global G;
UserDef U;

/* -------------------------------------------------------------------- */
/** \name Global Main Methods
 * \{ */

extern const Theme U_theme_default;
extern const Theme U_theme_xp;

void KER_rose_free() {
	KER_rose_globals_clear();
}

void KER_rose_userdef_init() {
	Theme *const themes[] = {&U_theme_default, &U_theme_xp};

	for (size_t index = 0; index < ARRAY_SIZE(themes); index++) {
		Theme *theme = MEM_mallocN(sizeof(Theme), "DefaultTheme");
		memcpy(theme, themes[index], sizeof(Theme));
		LIB_addtail(&U.themes, theme);
	}
}
void KER_rose_userdef_clear() {
	KER_userdef_clear(&U);
}

void KER_rose_globals_init() {
	memset(&G, 0, sizeof(Global));
}
void KER_rose_globals_clear() {
	if (G_MAIN == NULL) {
		return;
	}
	ROSE_assert(G_MAIN->is_global_main);
	KER_main_free(G_MAIN);

	G_MAIN = NULL;
}

void KER_rose_globals_main_replace(Main *main) {
	ROSE_assert(!main->is_global_main);
	KER_rose_globals_clear();
	main->is_global_main = true;
	G_MAIN = main;
}
Main *KER_rose_globals_main_swap(Main *nmain) {
	Main *pmain = G_MAIN;
	ROSE_assert(pmain->is_global_main);
	ROSE_assert(!nmain->is_global_main);
	nmain->is_global_main = true;
	G_MAIN = nmain;
	pmain->is_global_main = false;
	return pmain;
}

/** \} */
