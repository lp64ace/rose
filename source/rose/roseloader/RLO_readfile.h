#ifndef RLO_READFILE_H
#define RLO_READFILE_H

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#include "DNA_space_types.h"

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
	struct OldNewMap *map_lib;

	/** Used for relative paths handling. */
	char relabase[FILE_MAX];

	struct SDNA *f_dna;
	struct SDNA *m_dna;
	struct Main *main;
	struct ListBase *mainlist;

	int flag;

	size_t f_ptr_size;
	size_t m_ptr_size;
	size_t id_name_offset;
} FileData;

/** #FileData->flag */
enum {
	FD_FLAG_SWITCH_ENDIAN = 1 << 0,
	FD_FLAG_FILE_POINTSIZE_IS_4 = 1 << 1,
	FD_FLAG_POINTSIZE_DIFFERS = 1 << 2,
	FD_FLAG_FILE_OK = 1 << 3,
	FD_FLAG_IS_MEMFILE = 1 << 4,
	FD_FLAG_HAS_INVALID_ID_NAMES = 1 << 5,
};

struct RoseFileData *RLO_read_from_file(const char *filepath, int flag);
// struct RoseFileData *RLO_read_from_memory(const void *memory, size_t length, int flag);

void RLO_rosefile_data_free(struct RoseFileData *rfd);

#ifdef __cplusplus
}
#endif

#endif	// RLO_READFILE_H
