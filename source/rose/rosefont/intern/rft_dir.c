#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "LIB_fileops.h"
#include "LIB_string.h"

#include "MEM_alloc.h"

char *rft_dir_metrics_search(const char *filepath) {
	char *mfile, *s;

	mfile = LIB_strdupN(filepath);
	s = strrchr(mfile, '.');
	if (s) {
		if (LIB_strnlen(s, 4) < 4) {
			MEM_freeN(mfile);
			return NULL;
		}
		memcpy(s, ".afm", sizeof(char[4]));
		if (LIB_exists(mfile)) {
			return mfile;
		}
		memcpy(s, ".pfm", sizeof(char[4]));
		if (LIB_exists(mfile)) {
			return mfile;
		}
	}
	MEM_freeN(mfile);
	return NULL;
}
