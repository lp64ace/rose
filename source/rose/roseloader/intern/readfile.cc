#include "MEM_guardedalloc.h"

#include "DNA_sdna_types.h"
#include "DNA_userdef_types.h"

#include "LIB_assert.h"
#include "LIB_endian_switch.h"
#include "LIB_implicit_sharing.hh"
#include "LIB_fileops.h"
#include "LIB_filereader.h"
#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_map.hh"
#include "LIB_path_utils.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_anim_data.h"
#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_idprop.h"
#include "KER_lib_id.h"
#include "KER_main.h"
#include "KER_main_name_map.h"
#include "KER_rosefile.h"
#include "KER_userdef.h"

#include "RLO_read_write.hh"
#include "RLO_readfile.h"

#include "RT_parser.h"

#include "intern/genfile.h"	 // DNA

#include <limits.h>
#include <stdio.h>

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

ROSE_STATIC void *oldnewmap_liblookup(OldNewMap *onm, uint64_t addr, const void *lib) {
	if (!addr) {
		return NULL;
	}

	ID *id = static_cast<ID *>(oldnewmap_lookup_and_inc(onm, addr, false));
	if (id == NULL) {
		return NULL;
  	}
	if (!lib || id->lib) {
		return id;
	}
	return NULL;
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

ROSE_STATIC void *newlibaddr(FileData *fd, const void *lib, uint64_t address) {
	return oldnewmap_liblookup(fd->map_lib, address, lib);
}

ROSE_STATIC void *newataddr_no_us(FileData *fd, uint64_t address) {
	return oldnewmap_lookup_and_inc(fd->map_data, address, false);
}

ROSE_STATIC void change_link_placeholder_to_real_ID_pointer_fd(FileData *fd, const void *oldptr, void *newptr) {
	for (NewAddress &entry : fd->map_lib->map.values()) {
		if (oldptr == entry.newp && entry.nr == ID_LINK_PLACEHOLDER) {
			entry.newp = newptr;
			if (newptr) {
				entry.nr = GS(((ID *)newptr)->name);
			}
		}
	}
}

ROSE_STATIC void change_link_placeholder_to_real_ID_pointer(ListBase *mainlist, FileData *basefd, void *oldptr, void *newptr) {
	LISTBASE_FOREACH (Main *, mainptr, mainlist) {
		FileData *fd;

		if (mainptr->curlib) {
			fd = mainptr->curlib->runtime.filedata;
		}
		else {
			fd = basefd;
		}

		if (fd) {
			change_link_placeholder_to_real_ID_pointer_fd(fd, oldptr, newptr);
		}
	}
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

ROSE_INLINE bool rlo_rhead_is_id(const RHead *head) {
	/* RHead codes are four bytes (like 'ENDB', 'TEST', etc.), but if the two most-significant bytes
     * are zero, the values actually indicate an ID type. */
	return head->filecode <= 0xFFFF;
}

ROSE_INLINE const char *rlo_rhead_id_name(FileData *fd, const RHead *head) {
	ROSE_assert(rlo_rhead_is_id(head));
	const char *id_name = reinterpret_cast<const char *>(POINTER_OFFSET(head, sizeof(*head) + fd->id_name_offset));
	if (std::memchr(id_name, '\0', MAX_ID_NAME)) {
		return id_name;
	}

	fd->flag |= FD_FLAG_HAS_INVALID_ID_NAMES;
	return NULL;
}

typedef struct RoseDataReader {
	FileData *fd;

	/**
	 * The key is the old address id referencing shared data that's written to a file, typically an
	 * array. The corresponding value is the shared data at run-time.
	 */
	rose::Map<uint64_t, rose::ImplicitSharingInfoAndData> shared_data_by_stored_address;
} RoseDataReader;

typedef struct RoseLibReader {
	FileData *fd;
	Main *main;
} RoseLibReader;

/** \} */

/* -------------------------------------------------------------------- */
/** \name DNA Struct Loading
 * \{ */

ROSE_INLINE void switch_endian_structs(const SDNA *sdna, RHead *head) {
	void *data = head + 1;
	size_t count = head->length, size = DNA_sdna_struct_size(sdna, head->dnatype);
	while (count--) {
		DNA_struct_switch_endian(sdna, head->dnatype, data);

		data = POINTER_OFFSET(data, size);
	}
}

ROSE_INLINE void *read_struct(FileData *fd, RHead *head, const char *blockname) {
	void *temp = NULL;

	if (head->size) {
		if (head->dnatype && (fd->flag & FD_FLAG_SWITCH_ENDIAN) != 0) {
			switch_endian_structs(fd->f_dna, head);
		}

		if (KER_idtype_idcode_is_valid(head->filecode)) {
			int mask = (1 << (1 + sizeof(short) * 8)) - 1;
			if ((head->filecode & mask) == head->filecode) {
				blockname = KER_idtype_idcode_to_name((short)head->filecode);
			}
		}

		if (head->dnatype) {
			temp = DNA_sdna_struct_reconstruct(fd->f_dna, fd->m_dna, head->dnatype, head->length, head + 1, blockname);
		}
		else {
			temp = MEM_callocN(head->size, blockname);
			memcpy(temp, head + 1, head->size);
		}
	}

	return temp;
}

typedef void (*link_list_cb)(FileData *fd, void *data);

ROSE_INLINE void read_list_ex(FileData *fd, ListBase *lb, link_list_cb callback) {
	Link *ln, *prev;

	if (LIB_listbase_is_empty(lb)) {
		return;
	}

	lb->first = newataddr(fd, (uint64_t)lb->first);

	if (callback != NULL) {
		callback(fd, lb->first);
	}

	ln = (Link *)lb->first;
	prev = NULL;
	while (ln) {
		ln->next = (Link *)newataddr(fd, (uint64_t)ln->next);
		if (ln->next != NULL && callback != NULL) {
			callback(fd, ln->next);
		}
		ln->prev = prev;
		prev = ln;
		ln = ln->next;
	}
	lb->last = prev;
}

void RLO_read_list(RoseDataReader *reader, ListBase *lb) {
	read_list_ex(reader->fd, lb, NULL);
}

rose::ImplicitSharingInfoAndData rlo_read_shared_impl(RoseDataReader *reader, const void **ptr, rose::FunctionRef<const ImplicitSharingInfoHandle *()> read_fn) {
	uint64_t old_address = (uint64_t)*ptr;

	// TOOD; Handle UNDO

	if (const rose::ImplicitSharingInfoAndData *shared_data = reader->shared_data_by_stored_address.lookup_ptr(old_address)) {
		if (shared_data->sharing_info) {
			shared_data->sharing_info->add_user();
		}
		return *shared_data;
	}

	const rose::ImplicitSharingInfo *sharing_info = read_fn();
	const rose::ImplicitSharingInfoAndData shared_data = {sharing_info, *ptr};
	reader->shared_data_by_stored_address.add(old_address, shared_data);
	return shared_data;
}

const ImplicitSharingInfoHandle *RLO_read_shared(RoseDataReader *reader, void **data, rose::FunctionRef<const ImplicitSharingInfoHandle *()> read_fn) {
	rose::ImplicitSharingInfoAndData shared_data = rlo_read_shared_impl(reader, (const void **)data, read_fn);
	*data = const_cast<void *>(shared_data.data);
	return shared_data.sharing_info;
}

ROSE_INLINE void test_pointer_array(FileData *fd, void **mat) {
	int64_t *lpoin, *lmat;
	int *ipoin, *imat;

	/* manually convert the pointer array in the old dna format to a pointer array in the new dna format. */
	if (*mat) {
		size_t length = MEM_allocN_length(*mat) / fd->f_ptr_size;

		if (fd->f_ptr_size == 8 && fd->m_ptr_size == 4) {
			ipoin = imat = (int *)MEM_mallocN(length * fd->m_ptr_size, "newmatar");
      		lpoin = (int64_t *)*mat;

			while (length-- > 0) {
				if ((fd->flag & FD_FLAG_SWITCH_ENDIAN)) {
					LIB_endian_switch_int64(lpoin);
				}
				*ipoin = (int)((*lpoin) >> 3);
				ipoin++;
				lpoin++;
			}
			MEM_freeN(*mat);
			*mat = imat;
		}

		if (fd->f_ptr_size == 4 && fd->m_ptr_size == 8) {
			lpoin = lmat = (int64_t *)MEM_mallocN(length * fd->m_ptr_size, "newmatar");
      		ipoin = (int *)*mat;

			while (length-- > 0) {
				*lpoin = *ipoin;
				ipoin++;
				lpoin++;
			}
			MEM_freeN(*mat);
			*mat = lmat;
		}
	}
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

ROSE_STATIC void do_versions(FileData *fd, Library *lib, Main *main) {

}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read ID Library
 * \{ */

ROSE_STATIC void direct_link_library(FileData *fd, Library *lib, Main *main) {
	Main *newmain;

	/* check if the library was already read */
	for (newmain = (Main *)fd->mainlist->first; newmain; newmain = newmain->next) {
		if (newmain->curlib) {
			if (LIB_path_cmp(newmain->curlib->filepath, lib->filepath) == 0) {
				change_link_placeholder_to_real_ID_pointer(fd->mainlist, fd, lib, newmain->curlib);
				LIB_remlink(&main->libraries, lib);
				MEM_freeN(lib);

				LIB_remlink(fd->mainlist, newmain);
        		LIB_addtail(fd->mainlist, newmain);

				return;
			}
		}
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read ID
 * \{ */

ROSE_STATIC void lib_link_id(RoseLibReader *reader, ID *id) {
	IDP_RoseReadLib(reader, id->properties);

	AnimData *adt = KER_animdata_from_id(id);
	if (adt != NULL) {
		KER_animdata_rose_read_lib(reader, id, adt);
	}
}

ROSE_STATIC void lib_link_library(RoseLibReader *reader, Library *lib) {
	// No-op
}

ROSE_STATIC void lib_link_all(FileData *fd, Main *main) {
	RoseLibReader reader = {fd, main};

	ID *id;
	FOREACH_MAIN_ID_BEGIN(main, id) {
		if ((id->tag & ID_TAG_NEED_LINK) == 0) {
			/* This ID does not need liblink, just skip to next one. */
			continue;
		}

		lib_link_id(&reader, id);

		const IDTypeInfo *id_type = KER_idtype_get_info_from_id(id);
		if (id_type->read_lib != NULL) {
			id_type->read_lib(&reader, id);
		}

		if (GS(id->name) == ID_LI) {
			lib_link_library(&reader, (Library *)id);
		}

		id->tag &= ~ID_TAG_NEED_LINK;
	}
	FOREACH_MAIN_ID_END;

	/* Cleanup `ID.orig_id`, this is now reserved for depsgraph/COW usage only. */
	FOREACH_MAIN_ID_BEGIN (main, id) {
		id->orig_id = NULL;
	}
	FOREACH_MAIN_ID_END;
}

ROSE_STATIC void direct_link_id_common(RoseDataReader *reader, Library *curlib, ID *id) {
	id->lib = curlib;
	id->user = ID_FAKE_USERS(id);
	id->newid = NULL; /* Needed because .rose may have been saved with crap value here... */
	id->orig_id = NULL;

	if (id->properties) {
		RLO_read_data_address(reader, &id->properties);
		IDP_RoseReadData(reader, &id->properties, __func__);
	}
}

ROSE_STATIC void direct_link_id(FileData *fd, Main *main, ID *id) {
	RoseDataReader reader = {fd};
	
	direct_link_id_common(&reader, main->curlib, id);

	const IDTypeInfo *id_type = KER_idtype_get_info_from_id(id);
	if (id_type->read_data != NULL) {
		id_type->read_data(&reader, id);
	}

	id->tag |= ID_TAG_NEED_LINK;

	switch (GS(id->name)) {
		case ID_LI: {
			direct_link_library(fd, (Library *)id, main);
		} break;
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Read Lib-Blocks
 * \{ */

ROSE_INLINE const char *dataname(short id_code) {
	switch (id_code) {
		case ID_OB: return "Data from OB";
		case ID_ME: return "Data from ME";
		case ID_SCE: return "Data from SCE";
		case ID_MA: return "Data from MA";
		case ID_GR: return "Data from GR";
		case ID_AR: return "Data from AR";
		case ID_AC: return "Data from AC";
		case ID_LI: return "Data from LI";
		case ID_CA: return "Data from CA";
		case ID_SCR: return "Data from SCR";
		case ID_WM: return "Data from WM";
	}
	
	/** Not really a big problem/issue but we should keep this consistent with new IDTypes. */
	ROSE_assert_unreachable();

	return "Data from Lib Block";
}

ROSE_INLINE RHead *read_libblock(FileData *fd, Main *main, RHead *head, ID **r_id) {
	/* This routine reads a libblock and its direct data. Use link function to connect it all */
	ID *id = reinterpret_cast<ID *>(read_struct(fd, head, "libblock"));

	if (id == NULL) {
		if (r_id) {
			*r_id = NULL;
		}
		return rlo_rhead_next(fd, head);
	}

	const short idcode = GS(id->name);
	/* do after read_struct, for dna reconstruct. */
	ListBase *lb = which_libbase(main, idcode);
	if (lb == NULL) {
		/* unknown ID type */
		printf("%s: unkown id code '%c%c'\n", __func__, (idcode & 0xff), (idcode >> 8));
		MEM_freeN(id);

		if (r_id) {
			*r_id = NULL;
		}
		return rlo_rhead_next(fd, head);
	}

	ROSE_assert(idcode != ID_GR);

	LIB_addtail(lb, id);

	oldnewmap_insert(fd->map_lib, head->address, id, head->filecode);

	if (r_id) {
		*r_id = id;
	}

	id->lib = main->curlib;
	id->user = ID_FAKE_USERS(id);
	id->newid = NULL;
	id->orig_id = NULL;
	id->recalc = 0;

	if (head->filecode == ID_LINK_PLACEHOLDER) {
		return rlo_rhead_next(fd, head);
	}

	/* need a name for the mallocN, just for debugging and sane prints on leaks */
	const char *allocname = dataname(GS(id->name));
	/* read all data into fd->datamap */
  	head = read_data_into_datamap(fd, head, allocname);

	/* init pointers direct data */
	direct_link_id(fd, main, id);

	oldnewmap_clear(fd->map_data);
	return head;
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
	} while (false);

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

static void convert_pointer_array_64_to_32(RoseDataReader *reader, uint array_size, const uint64_t *src, uint32_t *dst) {
	if (RLO_read_requires_endian_switch(reader)) {
		for (int i = 0; i < array_size; i++) {
			uint64_t ptr = src[i];
			LIB_endian_switch_uint64(&ptr);
			dst[i] = (uint32_t)(ptr >> 3);
		}
	}
	else {
		for (int i = 0; i < array_size; i++) {
			dst[i] = (uint32_t)(src[i] >> 3);
		}
	}
}

ROSE_INLINE void convert_pointer_array_32_to_64(RoseDataReader *reader, uint array_size, const uint32_t *src, uint64_t *dst) {
	for (int i = 0; i < array_size; i++) {
		dst[i] = src[i];
	}
}

void RLO_read_pointer_array(RoseDataReader *reader, int length, void **ptr_p) {
	FileData *fd = reader->fd;

	void *orig_array = newataddr(fd, (uint64_t)*ptr_p);
	if (orig_array == NULL) {
		*ptr_p = NULL;
		return;
	}

	int array_size = MEM_allocN_length(orig_array) / fd->f_ptr_size;

	ROSE_assert(array_size == length);

	void *final_array = NULL;
	if (fd->f_ptr_size == fd->m_ptr_size) {
		final_array = orig_array;
	}
	else if (fd->f_ptr_size == 8 && fd->m_ptr_size == 4) {
		final_array = MEM_mallocN(fd->m_ptr_size * array_size, "new pointer array");
		convert_pointer_array_64_to_32(reader, array_size, (const uint64_t *)orig_array, (uint32_t *)final_array);
		MEM_freeN(orig_array);
	}
	else if (fd->f_ptr_size == 4 && fd->m_ptr_size == 8) {
		final_array = MEM_mallocN(fd->m_ptr_size * array_size, "new pointer array");
		convert_pointer_array_32_to_64(reader, array_size, (const uint32_t *)orig_array, (uint64_t *)final_array);
		MEM_freeN(orig_array);
	}

	*ptr_p = final_array;
}

ID *RLO_read_get_new_id_address(RoseLibReader *reader, Library *lib, ID *id) {
	return static_cast<ID *>(newlibaddr(reader->fd, lib, (uint64_t)id));
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
	fd->map_lib = oldnewmap_new();

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
	oldnewmap_free(fd->map_lib);
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

ROSE_INLINE size_t read_dna_struct_field_offset(SDNA *sdna, const char *structname, const char *fieldname) {
	const DNATypeStruct *dnatype = (const DNATypeStruct *)DNA_sdna_type(sdna, "ID");
	if (DNA_sdna_type_kind(sdna, (DNAType *)dnatype) != DNA_STRUCT) {
		return 0;
	}

	const DNATypeStructField *field = DNA_sdna_struct_field_find(sdna, dnatype, "name");
	if (field == NULL) {
		return 0;
	}

	return DNA_sdna_offsetof(sdna, dnatype, field);
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

			const DNAType *f_size = DNA_sdna_type(fd->f_dna, "conf::tp_size");
			const DNAType *m_size = DNA_sdna_type(fd->m_dna, "conf::tp_size");

			fd->f_ptr_size = DNA_sdna_sizeof(fd->f_dna, f_size);
			fd->m_ptr_size = DNA_sdna_sizeof(fd->m_dna, m_size);
			fd->id_name_offset = read_dna_struct_field_offset(fd->f_dna, "ID", "name");

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

/* -------------------------------------------------------------------- */
/** \name Helper Functions
 * \{ */

ROSE_STATIC void add_main_to_main(Main *mainvar, Main *from) {
	ListBase *lbarray[INDEX_ID_MAX], *fromarray[INDEX_ID_MAX];
	int a;

	set_listbasepointers(mainvar, lbarray);
	a = set_listbasepointers(from, fromarray);
	while (a--) {
		LIB_move_list_to_list(lbarray[a], fromarray[a]);
	}
}

void rlo_join_main(ListBase *mainlist) {
	Main *tojoin, *mainl;

	mainl = (Main *)mainlist->first;
	if (mainl->id_map != NULL) {
    	/* Cannot keep this since we add some IDs from joined mains. */
		KER_main_namemap_destroy(&mainl->name_map);
		mainl->id_map = NULL;
	}

	while ((tojoin = mainl->next)) {
		add_main_to_main(mainl, tojoin);
		LIB_remlink(mainlist, tojoin);
		KER_main_free(tojoin);
	}
}

ROSE_STATIC void split_libdata(ListBase *lb_src, Main **lib_main_array, const size_t lib_main_array_length) {
	for (ID *id = reinterpret_cast<ID *>(lb_src->first), *idnext; id; id = idnext) {
		idnext = reinterpret_cast<ID *>(id->next);

		if (id->lib) {
			if ((id->lib->runtime.index < lib_main_array_length) && (lib_main_array[id->lib->runtime.index]->curlib == id->lib)) {
				Main *mainvar = lib_main_array[id->lib->runtime.index];
				ListBase *lb_dst = which_libbase(mainvar, GS(id->name));
				LIB_remlink(lb_src, id);
				LIB_addtail(lb_dst, id);
			}
		}
	}
}

ROSE_STATIC void rlo_split_main(ListBase *mainlist, Main *mainvar) {
	mainlist->first = mainlist->last = mainvar;
	mainvar->next = NULL;

	if (LIB_listbase_is_empty(&mainvar->libraries)) {
		return;
	}

	const size_t lib_main_array_length = LIB_listbase_count(&mainvar->libraries);
	Main **lib_main_array = reinterpret_cast<Main **>(MEM_mallocN(sizeof(*lib_main_array) * lib_main_array_length, __func__));

	size_t index = 0;
	LISTBASE_FOREACH_INDEX(Library *, lib, &mainvar->libraries, index) {
		Main *libmain = KER_main_new();
		libmain->curlib = lib;
		LIB_addtail(mainlist, libmain);

		/** Not the best practice to do 3 things at the same line but looks better this way! */
		lib_main_array[lib->runtime.index = index] = libmain;
	}

	ListBase *lbarray[INDEX_ID_MAX];
	index = set_listbasepointers(mainvar, lbarray);
	while (index--) {
		ID *id = reinterpret_cast<ID *>(lbarray[index]->first);
		if (id == NULL || GS(id->name) == ID_LI) {
			/* No ID_LI data-lock should ever be linked anyway, but just in case, better be explicit. */
			continue;
		}

		split_libdata(lbarray[index], lib_main_array, lib_main_array_length);
	}

	MEM_freeN(lib_main_array);
}

ROSE_INLINE bool rlo_rhead_is_id_valid_type(const RHead *head) {
	if (!rlo_rhead_is_id(head)) {
		return false;
	}

	const short idcode = head->filecode & 0xFFFF;
	return KER_idtype_idcode_is_valid(idcode);
}

/** \} */

ROSE_STATIC RoseFileData *rlo_read_file_internal(FileData *fd, const char *filepath, int flag) {
	ListBase mainlist;
	LIB_listbase_clear(&mainlist);

	RoseFileData *rfd = static_cast<RoseFileData *>(MEM_callocN(sizeof(RoseFileData), "RoseFileData"));

	rfd->main = KER_main_new();
	fd->main = rfd->main;

	LIB_addtail(&mainlist, rfd->main);
	fd->mainlist = &mainlist;
	LIB_strcpy(rfd->main->filepath, ARRAY_SIZE(rfd->main->filepath), filepath);

	RHead *head = rlo_rhead_first(fd);

	while (head) {
		switch (head->filecode) {
			case RLO_CODE_DNA1:
			case RLO_CODE_DATA: {
				/** Skip this block for now... handled elsewhere! */
				head = rlo_rhead_next(fd, head);
			} break;
			case RLO_CODE_USER: {
				head = read_userdef(rfd, fd, head);
			} break;
			default: {
				head = read_libblock(fd, rfd->main, head, NULL);
			} break;
		}
	}

	do_versions(fd, NULL, rfd->main);
	do_versions_userdef(fd, rfd);

	rlo_join_main(&mainlist);

	lib_link_all(fd, rfd->main);

	KER_main_id_refcount_recompute(rfd->main, false);

	return rfd;
}

RoseFileData *RLO_read_from_file(const char *filepath, int flag) {
	RoseFileData *rfd = NULL;
	FileData *fd = rlo_filedata_from_file(filepath);
	if (fd) {
		rfd = rlo_read_file_internal(fd, filepath, flag);
		filedata_free(fd);
	}
	return rfd;
}

void RLO_rosefile_data_free(RoseFileData *rfd) {
	if (rfd->user) {
		KER_userdef_free(rfd->user);
	}
	if (rfd->main) {
		KER_main_free(rfd->main);
	}
	MEM_freeN(rfd);
}
