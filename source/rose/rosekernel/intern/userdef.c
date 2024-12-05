#include "MEM_guardedalloc.h"

#include "DNA_userdef_types.h"

#include "KER_userdef.h"

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void KER_rose_userdef_data_set_and_free(UserDef *userdef) {
	memswap(&U, userdef, sizeof(UserDef));
	KER_userdef_free(userdef);
}

void KER_userdef_clear(UserDef *userdef) {
	LIB_freelistN(&userdef->themes);
}

void KER_userdef_free(UserDef *userdef) {
	KER_userdef_clear(userdef);
	MEM_freeN(userdef);
}

/** \} */
