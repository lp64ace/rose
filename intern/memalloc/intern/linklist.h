#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Structures
 * \{ */

/**
 * Beware that these structures are also defined in other modules too, since we do not want to include these modules here we
 * are define them here, these headers (located in 'intern') should not be publicly included anyway so we should be safe from
 * any conflicts that might occur.
 */

typedef struct Link {
	struct Link *prev, *next;
} Link;

typedef struct ListBase {
	struct Link *first, *last;
} ListBase;

/* \} */

/* -------------------------------------------------------------------- */
/** \name Functions
 * \{ */

/**
 * Do not blow this too much, only insert function that are used in this module, more functions for #LinkList and #ListBase
 * located in the appropriate module (roselib).
 */

void addlink(struct ListBase *lb, struct Link *vlink);
void remlink(struct ListBase *lb, struct Link *vlink);

/* \} */

#ifdef __cplusplus
}
#endif
