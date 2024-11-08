#ifndef KER_GLOBAL_H
#define KER_GLOBAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef struct Global {
	/**
	 * Data for the current active rose file.
	 *
	 * Note that `CTX_data_main(C)` should be used where possible.
	 * Otherwise access via #G_MAIN.
	 */
	struct Main *main;
} Global;

/** \} */

/** Defined in `rose.c` */
extern Global G;

#ifdef __cplusplus
}
#endif

/**
 * Stupid macro to hide the few *valid* usages of `G.main` (from startup/exit code e.g.),
 * helps with cleanup task.
 */
#define G_MAIN (G).main

#endif	// KER_GLOBAL_H
