#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*IDTypeInitDataFunction)(struct ID *id);
typedef void (*IDTypeCopyDataFunction)(struct Main *main, struct ID *id_dst, const struct ID *id_src, int flag);
typedef void (*IDTypeFreeDataFunction)(struct ID *id);

typedef void (*IDTypeForeachIDFunction)(struct ID *id, struct LibraryForeachIDData *data);

typedef void (*IDTypeRoseWriteFunction)(struct RoseWriter *, struct ID *, const void *id_address);

typedef struct IDTypeInfo {
	/** Unique identifier of this type, either as a short or an array of two chars, see DNA_ID_enums.h's ID_XX enums. */
	short id_code;
	/** Bit-flag matching id_code, used for filtering (e.g. in file browser), see DNA_ID.h's FILTER_ID_XX enums. */
	uint64_t id_filter;
	/**
	 * Define the position of this data-block type in the virtual list of all data in a Main that is returned by
	 * `set_listbasepointers()`. Very important, this has to be unique and below INDEX_ID_MAX, see DNA_ID.h.
	 */
	int main_listbase_index;

	size_t struct_size;

	/** The user visible name for this data-block, also used as default name for a new data-block. */
	const char *name;
	/** Plural version of the user-visble name. */
	const char *name_plural;

	/** Initialize a new, empty calloc'ed data-block. May be NULL if there is nothing to do. */
	IDTypeInitDataFunction init_data;

	/**
	 * Copy the given data-block's data from source to destination. May be NULL if mere memcopy of the ID struct itself is
	 * enough.
	 */
	IDTypeCopyDataFunction copy_data;

	/** Free the data of the data-block (NOT the ID itself). May be NULL if there is nothing to do. */
	IDTypeFreeDataFunction free_data;
	
	/**
	 * Called by `KER_library_foreach_ID_link()` to apply a callback over all other ID usages of given data-block.
	 */
	IDTypeForeachIDFunction foreach_id;
	
	/**
	 * Write all structs that should be saved in a file.
	 */
	IDTypeRoseWriteFunction rose_write;
} IDTypeInfo;

/* -------------------------------------------------------------------- */
/** \name Retrieve State Information
 * \{ */

extern IDTypeInfo IDType_ID_LI;
extern IDTypeInfo IDType_ID_SCR;
extern IDTypeInfo IDType_ID_WS;
extern IDTypeInfo IDType_ID_WM;

/** Empty shell mostly, but needed for read code. */
extern IDTypeInfo IDType_ID_LINK_PLACEHOLDER;

/* \} */

/* -------------------------------------------------------------------- */
/** \name API
 * \{ */

void KER_idtype_init(void);

/* General helpers. */
const struct IDTypeInfo *KER_idtype_get_info_from_idcode(short id_code);
const struct IDTypeInfo *KER_idtype_get_info_from_id(const struct ID *id);

/** Returns the default name for the datablock from the \a idcode. */
const char *KER_idtype_idcode_to_name(short idcode);

/** Returns the default name plural for the datablock from the \a idcode. */
const char *KER_idtype_idcode_to_name_plural(short idcode);

/** Returns true if the idcode is valid. */
bool KER_idtype_idcode_is_valid(short idcode);

/** Returns the idcode from the name of a datablock. */
short KER_idtype_idcode_from_name(const char *idtype_name);

/** Convert an \a idcode into an \a idfilter (e.g. #ID_OB -> #FILTER_ID_OB). */
uint64_t KER_idtype_idcode_to_idfilter(short idcode);

/** Convert an \a idfilter into an \a idcode (e.g. #FILTER_ID_OB -> #ID_OB). */
short KER_idtype_idcode_from_idfilter(uint64_t idfilter);

/** Convert an \a idcode into an index (e.g. #ID_OB -> #INDEX_ID_OB). */
int KER_idtype_idcode_to_index(short idcode);

/** Get an \a idcode from an index (e.g. #INDEX_ID_OB -> #ID_OB). */
short KER_idtype_idcode_from_index(int index);

/** Return an ID code and steps the index forward 1. */
short KER_idtype_idcode_iter_step(int *index);

/* \} */

#ifdef __cplusplus
}
#endif
