#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void KER_rose_globals_init(void);
void KER_rose_globals_clear(void);

/** Replace current global Main by the given one, freeing existing one. */
void KER_rose_globals_main_replace(struct Main *main);
/**
 * Replace current global Main by the given one, returning the old one.
 *
 * \warning Advanced, risky workaround addressing the issue that current RNA is not able to process
 * correctly non-G_MAIN data, use with (a lot of) care.
 */
struct Main *KER_rose_globals_main_swap(struct Main *main);

#ifdef __cplusplus
}
#endif
