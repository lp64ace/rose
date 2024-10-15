#ifndef DNA_ID_H
#define DNA_ID_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

/**
 * ID structures represent generic data-blocks that are used by Rose. They are the base structure
 * of all major data-blocks!
 *
 * All ID data-blocks are either internal to Rose and stored in the global main database,
 * or belong to an external library. External library data-blocks are tagged as `LIB_TAG_EXTERN`,
 * indicating a weak reference to that data.
 *
 * Weak references imply that the data may not be available after a restart,
 * since the external library file might be missing, deleted, or altered,
 * leading to broken references.
 *
 * ID structures can also define properties, which are mainly used by higher-level users to extend
 * functionality (e.g., adding new variables or custom extensions).
 */
typedef struct ID {
	/**
	 * This is the base structure for other data-blocks, so we cannot define a specific type here!
	 * The `prev` and `next` pointers enable this structure to be part of a linked list of data-blocks.
	 */
	void *prev, *next;

	int flag;
	int tag;

	/** The first two bytes of the name are always used to determine the type of the ID. */
	char name[66];

	/** Pointer to additional properties, used for extended features. */
	struct IDProperty *properties;

	/**
	 * The library this data-block was loaded from. If NULL, the data-block is internal to the program
	 * (i.e., not loaded from an external library). This helps distinguish between internal and external
	 * data-blocks.
	 */
	struct Library *lib;
} ID;

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID2(a, b) ((a) << 8 | (b))
#else
/* Little Endian */
#	define MAKE_ID2(a, b) ((b) << 8 | (a))
#endif

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DNA_ID_H
