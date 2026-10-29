#include "MEM_guardedalloc.h"

#include "DNA_userdef_types.h"

#include "LIB_string.h"

#include "KER_global.h"
#include "KER_main.h"
#include "KER_rose.h"
#include "KER_userdef.h"

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
	const Theme *const themes[] = {&U_theme_default, &U_theme_xp};

	for (size_t index = 0; index < ARRAY_SIZE(themes); index++) {
		Theme *theme = MEM_mallocN(sizeof(Theme), "DefaultTheme");
		memcpy(theme, themes[index], sizeof(Theme));
		LIB_addtail(&U.themes, theme);
	}

	U.view_rotate_sensitivity_turntable = 5e-3f;

	// We could honestly leave this empty, and make workbench the default but I prefer basic to be the default!
	LIB_strcpy(U.engine, ARRAY_SIZE(U.engine), "ROSE_WORKBENCH");
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

/* -------------------------------------------------------------------- */
/** \name Rose's AtExit
 *
 * \note Don't use MEM_mallocN so functions can be registered at any time.
 * \{ */

typedef struct AtExitData {
	struct AtExitData *next;

	void (*func)(void *user_data);
	void *user_data;
} AtExitData;

static AtExitData *GAtExit = NULL;

void KER_rose_atexit_register(void (*func)(void *user_data), void *user_data) {
	struct AtExitData *ae = MEM_mallocN(sizeof(*ae), "AtExitData");
	ae->next = GAtExit;
	ae->func = func;
	ae->user_data = user_data;
	GAtExit = ae;
}

void KER_rose_atexit_unregister(void (*func)(void *user_data), const void *user_data) {
	struct AtExitData *ae = GAtExit;
	struct AtExitData **ae_p = &GAtExit;

	while (ae) {
		if ((ae->func == func) && (ae->user_data == user_data)) {
			*ae_p = ae->next;
			MEM_freeN(ae);
			return;
		}
		ae_p = &ae->next;
		ae = ae->next;
	}
}

void KER_rose_atexit(void) {
	struct AtExitData *ae = GAtExit, *next;
	while (ae) {
		next = ae->next;
		ae->func(ae->user_data);
		MEM_freeN(ae);
		ae = next;
	}
}

/** \} */
