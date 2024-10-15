#ifndef KER_IDTYPE_H
#define KER_IDTYPE_H

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct IDTypeInfo;
struct Main;
struct RoseWriter;
struct RoseDataReader;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef void (*IDTypeInitDataFunction)(ID *id);
typedef void (*IDTypeCopyDataFunction)(Main *main, ID *dst, const ID *src, int flag);
typedef void (*IDTypeFreeDataFunction)(ID *id);

typedef void (*IDTypeRoseWriteFunction)(RoseWriter *writer, ID *id, const void *address);
typedef void (*IDTypeRoseReadDataFunction)(RoseDataReader *reader, ID *id);

typedef struct IDTypeInfo {

	/* -------------------------------------------------------------------- */
	/** \name General IDType Data.
	 * \{ */

	int type;

	/** Bit-flag matching id_code, used for filtering, see `FILTER_ID_XX` enums. */
	uint64_t filter;
	/** Filter value containing only ID types that given ID could be using. */
	uint64_t depends;

	/**
	 * Defines the position of this data-block type in the virtual list of all data in a Main,
	 * the list is returned by `set_listbasepointers`.
	 */
	int index;

	size_t size;

	/** The user-visible name of this data-block, also used as default name for a new data-block. */
	const char *name;
	/** Plural version of the user-visible name. */
	const char *name_plural;

	int flag;

	/** \} */

	/* -------------------------------------------------------------------- */
	/** \name ID Management Callbacks.
	 * \{ */

	/**
	 * Initialize a new, empty calloc'ed data-block. May be NULL if there is nothing to do.
	 */
	IDTypeInitDataFunction init_data;

	/**
	 * Copy the given data-block's data from source to destination.
	 * May be NULL if mere memory-copy of the ID struct itself is enough.
	 */
	IDTypeCopyDataFunction copy_data;

	/**
	 * Free the data of the data-block (NOT the ID itself). May be NULL if there is nothing to do.
	 */
	IDTypeFreeDataFunction free_data;

	/** \} */

	/* -------------------------------------------------------------------- */
	/** \name Read & Write Callbacks.
	 * \{ */

	/**
	 * Write all structs that should be saved in a .rose file.
	 */
	IDTypeRoseWriteFunction write;

	/**
	 * Update pointers for all structs directly owned by this data block.
	 */
	IDTypeRoseReadDataFunction read_data;

	/** \} */

} IDTypeInfo;

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_IDTYPE_H
