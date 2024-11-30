#include "MEM_guardedalloc.h"

#include "DNA_sdna_types.h"
#include "DNA_userdef_types.h"

#include "LIB_assert.h"
#include "LIB_endian_switch.h"
#include "LIB_fileops.h"
#include "LIB_filereader.h"
#include "LIB_listbase.h"
#include "LIB_map.hh"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_global.h"
#include "KER_main.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_rosefile.h"
#include "KER_userdef.h"

#include "RLO_read_write.h"
#include "RLO_readfile.h"

#include "RT_parser.h"

#include "intern/genfile.h" // DNA

#include <stdio.h>
#include <limits.h>

/* -------------------------------------------------------------------- */
/** \name OldNewMap API
 * \{ */

typedef struct NewAddress {
	void *newp;
	
	int nr;
} NewAddress;

typedef struct OldNewMap {
	rose::Map<uint64_t, NewAddress> map;
} OldNewMap;

ROSE_STATIC OldNewMap *oldnewmap_new() {
	return MEM_new<OldNewMap>("OldNewMap");
}

ROSE_STATIC bool oldnewmap_insert(OldNewMap *onm, uint64_t oldaddr, void *newaddr, int nr) {
	if (!oldaddr || !newaddr) {
		return false;
	}
	return onm->map.add_overwrite(oldaddr, NewAddress{newaddr, nr});
}

ROSE_STATIC void *oldnewmap_lookup_and_inc(OldNewMap *onm, uint64_t oldaddr, bool increase_users) {
	NewAddress *entry = onm->map.lookup_ptr(oldaddr);
	if (entry == NULL) {
		return NULL;
	}
	entry->nr++;
	return entry->newp;
}

ROSE_STATIC void oldnewmap_clear(OldNewMap *onm) {
	for (NewAddress &new_addr : onm->map.values()) {
		if (new_addr.nr == 0) {
			MEM_freeN(new_addr.newp);
		}
	}
	onm->map.clear_and_shrink();
}

ROSE_STATIC void oldnewmap_free(OldNewMap *onm) {
	MEM_delete(onm);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Old/New Map
 * \{ */

ROSE_STATIC void *newataddr(FileData *fd, uint64_t address) {
	return oldnewmap_lookup_and_inc(fd->map_data, address, true);
}

ROSE_STATIC void *newataddr_no_us(FileData *fd, uint64_t address) {
	return oldnewmap_lookup_and_inc(fd->map_data, address, false);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Parsing
 * \{ */

typedef struct RHeadN {
	struct RHeadN *prev, *next;
	
	uint64_t offset;
	bool has_data;

	RHead head;
} RHeadN;

#define RHEADN_FROM_RHEAD(head) ((RHeadN *)POINTER_OFFSET((RHead *)head, -offsetof(RHeadN, head)))

ROSE_STATIC void switch_endian_rhead(RHead *head) {
	if (head->filecode != RLO_CODE_ENDB) {
		LIB_endian_switch_int32(&head->filecode);
		LIB_endian_switch_int32(&head->size);
		LIB_endian_switch_int32(&head->length);
		/** We don't really need to endian swap the address but might as well since we are at it... */
		LIB_endian_switch_uint64(&head->address);
		LIB_endian_switch_uint64(&head->dnatype);
	}
}

ROSE_STATIC RHeadN *get_rhead(FileData *fd) {
	RHeadN *nheadn = NULL;
	
	uint64_t readsize;
	
	if (fd) {
		RHead head;
		
		readsize = fd->file->read(fd->file, &head, sizeof(head));
		if (readsize == sizeof(head) && head.filecode != RLO_CODE_ENDB) {
			if ((fd->flag & FD_FLAG_SWITCH_ENDIAN) != 0) {
				switch_endian_rhead(&head);
			}
		}
		else {
			return NULL;
		}
		
		if (head.size < 0 || head.length < 0) {
			return NULL;
		}
		
		/** For now we read everything immediately! */
		nheadn = static_cast<RHeadN *>(MEM_mallocN(sizeof(RHeadN) + head.size, "RHeadN"));
		memcpy(&nheadn->head, &head, sizeof(RHead));
		nheadn->offset = fd->file->offset;
		
		readsize = fd->file->read(fd->file, nheadn + 1, head.size);
		if (readsize == head.size) {
			nheadn->has_data = true;
		}
		else {
			MEM_SAFE_FREE(nheadn);
		}
	}
	
	if (nheadn) {
		LIB_addtail(&fd->headlist, nheadn);
	}
	
	return nheadn;
}

RHead *rlo_rhead_first(FileData *fd) {
	RHeadN *nheadn = reinterpret_cast<RHeadN *>(fd->headlist.first);
	if (!nheadn) {
		nheadn = get_rhead(fd);
	}
	return (nheadn) ? &nheadn->head : NULL;
}

RHead *rlo_rhead_prev(FileData *fd, RHead *head) {
	RHeadN *nheadn = RHEADN_FROM_RHEAD(head);
	nheadn = nheadn->prev;
	return (nheadn) ? &nheadn->head : NULL;
}

RHead *rlo_rhead_next(FileData *fd, RHead *head) {
	if (!head) {
		return NULL;
	}
	RHeadN *nheadn = RHEADN_FROM_RHEAD(head);
	nheadn = nheadn->next;
	if (!nheadn) {
		nheadn = get_rhead(fd);
	}
	return (nheadn) ? &nheadn->head : NULL;
}

typedef struct RoseDataReader {
	FileData *fd;
} RoseDataReader;



/** \} */

/* -------------------------------------------------------------------- */
/** \name DNA Struct Loading
 * \{ */

ROSE_STATIC void switch_endian_structs(const SDNA *sdna, RHead *head) {
	void *data = head + 1;
	size_t count = head->length, size = DNA_sdna_struct_size(sdna, head->dnatype);
	while(count--) {
		DNA_struct_switch_endian(sdna, head->dnatype, data);
		
		data = POINTER_OFFSET(data, size);
	}
}

ROSE_STATIC void *read_struct(FileData *fd, RHead *head, const char *blockname) {
	void *temp = NULL;
	
	if (head->size) {
		if (head->dnatype && (fd->flag & FD_FLAG_SWITCH_ENDIAN) != 0) {
			switch_endian_structs(fd->f_dna, head);
		}

		temp = DNA_sdna_struct_reconstruct(fd->f_dna, fd->m_dna, head->dnatype, head + 1, blockname);
	}
	
	return temp;
}
 
/** \} */

/* -------------------------------------------------------------------- */
/** \name Data Loading
 * \{ */

ROSE_STATIC RHead *read_data_into_datamap(FileData *fd, RHead *head, const char *blockname) {
	head = rlo_rhead_next(fd, head);
	
	while (head && head->filecode == RLO_CODE_DATA) {
		void *data = read_struct(fd, head, blockname);
		if (data) {
			const bool is_new = oldnewmap_insert(fd->map_data, head->address, data, 0);
			ROSE_assert(is_new);
		}
		head = rlo_rhead_next(fd, head);
	}
	
	return head;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Versioning
 * \{ */

ROSE_STATIC void do_versions_userdef(FileData *fd, RoseFileData *rfd) {
	UserDef *user = rfd->user;
	
	if (user == NULL) {
		return;
	}
	
	rlo_do_versions_userdef(user);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read User Preferences
 * \{ */

ROSE_STATIC RHead *read_userdef(RoseFileData *rfd, FileData *fd, RHead *head) {
	UserDef *user;
	
	rfd->user = user = static_cast<UserDef *>(read_struct(fd, head, "UserDef"));
	
	head = read_data_into_datamap(fd, head, "UserDef::Data");
	
	RoseDataReader reader = {fd};
	do {
		/** Read all the data associated with #UserDef. */
		RLO_read_struct_list(&reader, Theme, &user->themes);
	} while(false);
	
	oldnewmap_clear(fd->map_data);
	
	return head;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose Read API
 * \{ */

bool RLO_read_requires_endian_switch(RoseDataReader *reader) {
	if ((reader->fd->flag & FD_FLAG_SWITCH_ENDIAN) != 0) {
		return true;
	}
	return false;
}

void *RLO_read_get_new_data_address(RoseDataReader *reader, const void *old_address) {
	return newataddr(reader->fd, (uint64_t)old_address);
}

void *RLO_read_get_new_data_address_no_user(RoseDataReader *reader, const void *old_address) {
	return newataddr_no_us(reader->fd, (uint64_t)old_address);
}

ROSE_STATIC void *rlo_verify_data_address(void *newp, const void *old_address, const size_t size) {
	if (newp != NULL) {
		ROSE_assert_msg(MEM_allocN_length(newp) >= size, "Corrupt .rose file, unexpected data size.");
		UNUSED_VARS_NDEBUG(size);
	}
	UNUSED_VARS(old_address);
	return newp;
}

void *RLO_read_struct_array_with_size(RoseDataReader *reader, const void *old_address, const size_t size) {
	void *newp = newataddr(reader->fd, (uint64_t)old_address);
	return rlo_verify_data_address(newp, old_address, size);
}

void RLO_read_struct_list_with_size(struct RoseDataReader *reader, size_t esize, ListBase *list) {
	if (LIB_listbase_is_empty(list)) {
		return;
	}
	
	list->first = reinterpret_cast<Link *>(RLO_read_struct_array_with_size(reader, list->first, esize));
	Link *ln = static_cast<Link *>(list->first);
	Link *prev = NULL;
	while (ln) {
		ln->next = static_cast<Link *>(RLO_read_struct_array_with_size(reader, ln->next, esize));
		ln->prev = prev;
		prev = ln;
		ln = ln->next;
	}
	list->last = prev;
}

void RLO_read_char_array(RoseDataReader *reader, int array_size, char **ptr_p) {
	*ptr_p = reinterpret_cast<char *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(char) * array_size));
}

void RLO_read_int8_array(RoseDataReader *reader, int array_size, int8_t **ptr_p) {
	*ptr_p = reinterpret_cast<int8_t *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(int8_t) * array_size));
}

void RLO_read_uint8_array(RoseDataReader *reader, int array_size, uint8_t **ptr_p) {
	*ptr_p = reinterpret_cast<uint8_t *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(uint8_t) * array_size));
}

void RLO_read_int32_array(RoseDataReader *reader, int array_size, int32_t **ptr_p) {
	*ptr_p = reinterpret_cast<int32_t *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(int32_t) * array_size));
	
	if (*ptr_p && RLO_read_requires_endian_switch(reader)) {
		LIB_endian_switch_int32_array(*ptr_p, array_size);
	}
}

void RLO_read_uint32_array(RoseDataReader *reader, int array_size, uint32_t **ptr_p) {
	*ptr_p = reinterpret_cast<uint32_t *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(uint32_t) * array_size));
	
	if (*ptr_p && RLO_read_requires_endian_switch(reader)) {
		LIB_endian_switch_uint32_array(*ptr_p, array_size);
	}
}

void RLO_read_float_array(RoseDataReader *reader, int array_size, float **ptr_p) {
	*ptr_p = reinterpret_cast<float *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(float) * array_size));
	
	if (*ptr_p && RLO_read_requires_endian_switch(reader)) {
		LIB_endian_switch_float_array(*ptr_p, array_size);
	}
}

void RLO_read_double_array(RoseDataReader *reader, int array_size, double **ptr_p) {
	*ptr_p = reinterpret_cast<double *>(RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(double) * array_size));
	
	if (*ptr_p && RLO_read_requires_endian_switch(reader)) {
		LIB_endian_switch_double_array(*ptr_p, array_size);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Data API
 * \{ */

ROSE_STATIC FileData *filedata_new(void) {
	FileData *fd = static_cast<FileData *>(MEM_mallocN(sizeof(FileData), "FileData"));
	
	LIB_listbase_clear(&fd->headlist);
	
	fd->f_dna = NULL;
	fd->m_dna = DNA_sdna_new_current();
	fd->flag = 0;

	fd->map_data = oldnewmap_new();
	fd->map_glob = oldnewmap_new();
	
	return fd;
}

ROSE_STATIC void filedata_free(FileData *fd) {
	if (fd->f_dna) {
		DNA_sdna_free(fd->f_dna);
	}
	if (fd->m_dna) {
		DNA_sdna_free(fd->m_dna);
	}
	LIB_freelistN(&fd->headlist);
	if (fd->file) {
		fd->file->close(fd->file);
	}
	oldnewmap_free(fd->map_data);
	oldnewmap_free(fd->map_glob);
	MEM_freeN(fd);
}

ROSE_STATIC void filedata_decode_rose_header(FileData *fd) {
	char header[8];
	
	uint64_t readsize = fd->file->read(fd->file, header, sizeof(header));
	
	if (readsize == sizeof(header) && STREQLEN(header, "ROSE", 4)) {
		/** The only reason we use two bytes for the endianess is to align everything into an 8byte string. */
		if (STREQLEN(header + 4, "BG", 2) || STREQLEN(header + 4, "LT", 2)) {
			if (ELEM(header[6] - '0', 4, 8) && ELEM(header[7], '\0')) {
				fd->flag |= FD_FLAG_FILE_OK;
			}
		}
	}
	
	if ((fd->flag & FD_FLAG_FILE_OK) != 0) {
		SET_FLAG_FROM_TEST(fd->flag, header[6] == '4', FD_FLAG_FILE_POINTSIZE_IS_4);
		if (header[6] != '0' + sizeof(void *)) {
			fd->flag |= FD_FLAG_POINTSIZE_DIFFERS;
		}
	}
	if ((fd->flag & FD_FLAG_FILE_OK) != 0) {
#ifdef __BIG_ENDIAN__
		SET_FLAG_FROM_TEST(fd->flag, header[4] == 'L', FD_FLAG_SWITCH_ENDIAN);
#else
		SET_FLAG_FROM_TEST(fd->flag, header[4] == 'B', FD_FLAG_SWITCH_ENDIAN);
#endif
	}
}

ROSE_STATIC bool read_file_dna(FileData *fd) {
	RHead *head;
	
	for (head = rlo_rhead_first(fd); head; head = rlo_rhead_next(fd, head)) {
		if (head->filecode == RLO_CODE_DNA1) {
			fd->f_dna = DNA_sdna_new_memory(&head[1], head->size);
			if (!fd->f_dna || !DNA_sdna_build_struct_list(fd->f_dna)) {
				fprintf(stderr, "Failed to read rose file '%s': %s\n", fd->relabase, "Invalid DNA");
				return false;
			}
			return true;
		}
	}
	
	fprintf(stderr, "Failed to read rose file '%s': %s\n", fd->relabase, "Missing DNA");
	return false;
}

ROSE_STATIC FileData *rlo_decode_and_check(FileData *fd) {
	filedata_decode_rose_header(fd);
	
	if ((fd->flag & FD_FLAG_FILE_OK) != 0) {
		if (!read_file_dna(fd)) {
			filedata_free(fd);
			return NULL;
		}
	}
	
	return fd;
}

ROSE_STATIC FileData *rlo_filedata_from_file_descriptor(int descr) {
	FileReader *rawfile = LIB_filereader_new_file(descr), *file = NULL;
	
	char header[4];
	if (rawfile == NULL || rawfile->read(rawfile, header, sizeof(header)) != sizeof(header)) {
		if (rawfile) {
			rawfile->close(rawfile);
		}
		else {
			close(descr);
		}
		return NULL;
	}
	
	rawfile->seek(rawfile, 0, SEEK_SET);
	
	if (memcmp(header, "ROSE", sizeof(header)) == 0) {
		SWAP(FileReader *, file, rawfile);
	}
	
	if (rawfile) {
		rawfile->close(rawfile);
	}
	
	if (file == NULL) {
		return NULL;
	}
	
	FileData *fd = filedata_new();
	fd->file = file;
	
	return fd;
}

ROSE_STATIC FileData *rlo_filedata_from_file_open(const char *filepath) {
	const int file = LIB_open(filepath, O_BINARY | O_RDONLY, 0);
	if (file < 0) {
		return NULL;
	}
	return rlo_filedata_from_file_descriptor(file);
}

ROSE_STATIC FileData *rlo_filedata_from_file(const char *filepath) {
	FileData *fd = rlo_filedata_from_file_open(filepath);
	if (fd != NULL) {
		LIB_strcpy(fd->relabase, ARRAY_SIZE(fd->relabase), filepath);
		
		return rlo_decode_and_check(fd);
	}
	return NULL;
}

/** \} */

void RLO_rosefile_data_free(RoseFileData *rfd) {
	if (rfd->user) {
		KER_userdef_free(rfd->user);
	}
	if (rfd->main) {
		KER_main_free(rfd->main);
	}
	MEM_freeN(rfd);
}

bool RLO_read_file(struct Main *main, const char *filepath, int flag) {
	FileData *fd = rlo_filedata_from_file(filepath);
	
	if (!fd) {
		return false;
	}
	
	RoseFileData *rfd = static_cast<RoseFileData *>(MEM_callocN(sizeof(RoseFileData), "RoseFileData"));
	
	RHead *head = rlo_rhead_first(fd);
	
	while(head) {
		switch (head->filecode) {
			case RLO_CODE_USER: {
				head = read_userdef(rfd, fd, head);
			} break;
			default: {
				head = rlo_rhead_next(fd, head);
			} break;
		}
	}
	
	do_versions_userdef(fd, rfd);
	KER_rosefile_read_setup(rfd);
	RLO_rosefile_data_free(rfd);
	
	filedata_free(fd);
	return true;
}
