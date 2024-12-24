#ifndef KER_IDTYPE_H
#define KER_IDTYPE_H

#include "LIB_utildefines.h"

#include "DNA_ID.h"
#include "DNA_ID_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct IDTypeInfo;
struct LibraryForeachIDData;
struct Main;
struct RoseWriter;
struct RoseDataReader;

/* -------------------------------------------------------------------- */
/** \name Data Structures
 * \{ */

typedef void (*IDTypeInitDataFunction)(struct ID *id);
typedef void (*IDTypeCopyDataFunction)(struct Main *main, struct ID *dst, const struct ID *src, int flag);
typedef void (*IDTypeFreeDataFunction)(struct ID *id);

typedef void (*IDTypeForeachIDFunction)(struct ID *id, struct LibraryForeachIDData *data);

typedef void (*IDTypeRoseWriteFunction)(struct RoseWriter *writer, struct ID *id, const void *address);
typedef void (*IDTypeRoseReadDataFunction)(struct RoseDataReader *reader, struct ID *id);

typedef struct IDTypeInfo {

	/* -------------------------------------------------------------------- */
	/** \name General IDType Data.
	 * \{ */

	short idcode;

	/** Bit-flag matching id_code, used for filtering, see `FILTER_ID_XX` enums. */
	int filter;
	/** Filter value containing only ID types that given ID could be using. */
	int depends;
	/**
	 * Defines the position of this data-block type in the virtual list of all data in a Main,
	 * the list is returned by `set_listbasepointers`.
	 */
	int index;
	/**
	 * The size to allocate for a single data block of this type,
	 * including the size of the base ID structure.
	 */
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
	/** \name Data Iteration Callbacks.
	 * \{ */

	IDTypeForeachIDFunction foreach_id;

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

enum {
	/** Indicates that the given IDType does not support copying. */
	IDTYPE_FLAGS_NO_COPY = 1 << 0,
};

/* -------------------------------------------------------------------- */
/** \name Static Types
 * \{ */

extern IDTypeInfo IDType_ID_LI;
extern IDTypeInfo IDType_ID_ME;
extern IDTypeInfo IDType_ID_OB;
extern IDTypeInfo IDType_ID_SCR;
extern IDTypeInfo IDType_ID_WM;

/** Empty shell mostly, but needed for read code. */
extern IDTypeInfo IDType_ID_LINK_PLACEHOLDER;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Main Methods
 * \{ */

void KER_idtype_init();

const struct IDTypeInfo *KER_idtype_get_info_from_idtype_index(const int idtype_index);
const struct IDTypeInfo *KER_idtype_get_info_from_idcode(short idcode);
const struct IDTypeInfo *KER_idtype_get_info_from_id(const struct ID *id);

const char *KER_idtype_idcode_to_name(short idcode);
const char *KER_idtype_idcode_to_name_plural(short idcode);

bool KER_idtype_idcode_is_valid(short idcode);

/**
 * Retrieve the ID type code from the user-visible name.
 *
 * \param idtype_name The user-visible name of the ID type to search for.
 * \return The corresponding ID code for the given name, or 0 if the name is invalid.
 */
short KER_idtype_idcode_from_name(const char *idtype_name);

int KER_idtype_idcode_to_index(short idcode);
int KER_idtype_idcode_to_idfilter(short idcode);
int KER_idtype_idfilter_to_index(int idfilter);
int KER_idtype_idfilter_to_idcode(int idfilter);
int KER_idtype_index_to_idcode(int idindex);
int KER_idtype_index_to_idfilter(int idindex);

/**
 * Retrieve the next ID code and advance the index by 1.
 *
 * \param idtype_index Pointer to the current index, starting at 0.
 * \return The next ID code, or 0 if all codes have been returned.
 */
short KER_idtype_idcode_iter_step(int *idtype_index);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_IDTYPE_H
