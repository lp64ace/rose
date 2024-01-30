#pragma once

typedef struct Global {
	/**
	 * The global main, this is the main of the current runtime project, prefer using `CTX_data_main` when applicable since
	 * this has the current's state main database.
	 */
	struct Main *main;
} Global;

/** Defined in `rose.c` */
extern Global G;

/**
 * Stupid macro to hide the few *valid* usages of `G.main` (from startup/exit code e.g.),
 * helps with cleanup task.
 */
#define G_MAIN (G).main
