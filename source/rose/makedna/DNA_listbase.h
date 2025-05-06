#ifndef DNA_LISTBASE_H
#define DNA_LISTBASE_H

#ifdef __cplusplus
extern "C" {
#endif

struct Link;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * The Link structure can be stored in a single ListBase.
 * It serves as the base structure for many data types, allowing
 * efficient manipulation of doubly linked lists like ListBase.
 */
typedef struct Link {
	/**
	 * Pointers to the previous and next links in the same ListBase.
	 * These pointers are undefined if this structure is not part of a list.
	 * \note Default initialization is zero; use MEM_callocN for memory allocation.
	 */
	struct Link *prev, *next;
} Link;

/**
 * ListBase is a doubly linked list that stores Link elements as nodes.
 */
typedef struct ListBase {
	/**
	 * Pointers to the first and last Link in this doubly linked list.
	 * Note that items cannot be shared between multiple ListBase lists.
	 * \note Default initialization is zero; use MEM_callocN for memory allocation.
	 */
	void *first, *last;
} ListBase;

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// DNA_LISTBASE_H
