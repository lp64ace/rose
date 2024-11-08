#ifndef KER_ROSE_H
#define KER_ROSE_H

#ifdef __cplusplus
extern "C" {
#endif

struct Main;

/* -------------------------------------------------------------------- */
/** \name Global Main Methods
 * \{ */

/**
 * Only to be called on exit Rose.
 */
void KER_rose_free();

void KER_rose_globals_init();
void KER_rose_globals_clear();

/** Replace current global Main by the given one, freeing existing one. */
void KER_rose_globals_main_replace(struct Main *main);
/**
 * Replace current global Main by the given one, returning the old one.
 *
 * \warning Advanced, risky workaround addressing the issue that current RNA is not able to process
 * correctly non-G_MAIN data, use with (a lot of) care.
 */
struct Main *KER_rose_globals_main_swap(struct Main *nmain);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_ROSE_H
