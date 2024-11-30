#ifndef RLO_READFILE_H
#define RLO_READFILE_H

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Main;
struct OldNewMap;
struct UserDef;

typedef struct FileData {
	ListBase headlist;

	struct FileReader *file;

	struct OldNewMap *map_data;
	struct OldNewMap *map_glob;

	/** Used for relative paths handling. */
	char relabase[1024];

	struct SDNA *f_dna;
	struct SDNA *m_dna;

	int flag;
} FileData;

/** #FileData->flag */
enum {
	FD_FLAG_SWITCH_ENDIAN = 1 << 0,
	FD_FLAG_FILE_POINTSIZE_IS_4 = 1 << 1,
	FD_FLAG_POINTSIZE_DIFFERS = 1 << 2,
	FD_FLAG_FILE_OK = 1 << 3,
	FD_FLAG_IS_MEMFILE = 1 << 4,
};

bool RLO_read_file(struct Main *main, const char *filepath, int flag);

#ifdef __cplusplus
}
#endif

#endif	// RLO_READFILE_H
