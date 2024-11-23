#include "DNA_userdef_types.h"

#include "KER_global.h"
#include "KER_main.h"
#include "KER_rose.h"

#include <string.h>

Global G;

extern const Theme U_theme_default;

UserDef U = {
	.themes = {
		.first = (Link *)&U_theme_default,
		.last = (Link *)&U_theme_default,
	},
};

/* -------------------------------------------------------------------- */
/** \name Global Main Methods
 * \{ */

void KER_rose_free() {
	KER_rose_globals_clear();
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
