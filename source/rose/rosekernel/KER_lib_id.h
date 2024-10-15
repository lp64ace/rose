#ifndef KER_LIB_ID_H
#define KER_LIB_ID_H

#include "DNA_ID.h"

#include "LIB_sys_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct IDProperty;
struct Library;

/* -------------------------------------------------------------------- */
/** \name Datablock Creation
 * \{ */

enum {
	LIB_ID_CREATE_NO_MAIN = 1 << 0,
	LIB_ID_CREATE_NO_USER_REFCOUNT = 1 << 1,
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Deletion
 * \{ */

/** \} */

/* -------------------------------------------------------------------- */
/** \name Datablock Reference Management
 * \{ */

void id_us_increment(struct ID *id);
void id_us_decrement(struct ID *id);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_LIB_ID_H
