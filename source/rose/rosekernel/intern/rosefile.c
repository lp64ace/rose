#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "KER_rosefile.h"
#include "KER_userdef.h"

#include "DNA_userdef_types.h"

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

ROSE_STATIC void setup_app_userdef(RoseFileData *rfd) {
	if (rfd->user) {
		KER_rose_userdef_data_set_and_free(rfd->user);
		rfd->user = NULL;
	}
}

ROSE_STATIC void setup_app_rose_file_data(RoseFileData *rfd) {
	setup_app_userdef(rfd);
}

void KER_rosefile_read_setup(RoseFileData *rfd) {
	setup_app_rose_file_data(rfd);
}

/** \} */
