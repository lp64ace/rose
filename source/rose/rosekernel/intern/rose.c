#pragma once

#include <string.h>

#include "LIB_assert.h"

#include "KER_context.h"
#include "KER_global.h"
#include "KER_main.h"
#include "KER_rose.h"

Global G;

void KER_rose_globals_init(void) {
	memset(&G, 0, sizeof(Global));

	KER_rose_globals_main_replace(KER_main_new());
}

void KER_rose_globals_clear(void) {
	if (G_MAIN == NULL) {
		return;
	}
	ROSE_assert(G_MAIN->is_global_main);
	KER_main_free(G_MAIN); /** Free all lib data. */

	G_MAIN = NULL;
}

void KER_rose_globals_main_replace(struct Main *main) {
	ROSE_assert(!main->is_global_main);
	KER_rose_globals_clear();
	main->is_global_main = true;
	G_MAIN = main;
}

struct Main *KER_rose_globals_main_swap(struct Main *main) {
	Main *prev = G_MAIN;
	ROSE_assert(prev->is_global_main);
	ROSE_assert(!main->is_global_main);
	main->is_global_main = true;
	G_MAIN = main;
	prev->is_global_main = false;
	return prev;
}
