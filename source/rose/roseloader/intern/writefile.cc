#include "MEM_guardedalloc.h"

#include "DNA_sdna_types.h"
#include "DNA_userdef_types.h"

#include "LIB_assert.h"
#include "LIB_fileops.h"
#include "LIB_listbase.h"
#include "LIB_memory_utils.hh"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"

#include "RLO_read_write.h"
#include "RLO_writefile.h"

#include "RT_parser.h"

#include "intern/genfile.h"	 // DNA

#include <limits.h>

/* -------------------------------------------------------------------- */
/** \name Internal Write Wrapper's (Abstracts Compression)
 * \{ */

class WriteWrap {
public:
	virtual bool open(const char *filepath) = 0;
	virtual bool close() = 0;
	virtual bool write(const void *but, size_t length) = 0;
};

class RawWriteWrap : public WriteWrap {
public:
	bool open(const char *filepath) override;
	bool close() override;
	bool write(const void *but, size_t lenth) override;

private:
	int fd = -1;
};

bool RawWriteWrap::open(const char *filepath) {
	int handle = LIB_open(filepath, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0666);
	return (handle >= 0) ? (this->fd = handle) >= 0 : false;
}

bool RawWriteWrap::close() {
	if (this->fd >= 0 && ::close(this->fd) == 0) {
		return true;
	}
	return false;
}

bool RawWriteWrap::write(const void *buffer, size_t length) {
	return ::write(this->fd, buffer, length) == length;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Write Data Type & Functions
 * \{ */

typedef struct WriteData {
	struct SDNA *dna;

	struct {
		/** Set on unlikely case of an error (ignores further file writing). */
		bool error;
	} validation;

	/** Wrap writing, so we can use zstd or other compression types later! */
	WriteWrap *ww;
} WriteData;

typedef struct RoseWriter {
	struct WriteData *wd;
} RoseWriter;

ROSE_INLINE WriteData *writedata_new(WriteWrap *ww) {
	WriteData *wd = MEM_cnew<WriteData>("WriteData");
	wd->dna = DNA_sdna_new_current();
	wd->ww = ww;
	return wd;
}

ROSE_INLINE void writedata_do_write(WriteData *wd, const void *mem, size_t length) {
	if ((wd == NULL) || (wd->validation.error != false)) {
		return;
	}

	if (length > INT_MAX) {
		ROSE_assert_msg(0, "Cannot write chunks bigger than INT_MAX!");
		return;
	}

	if (wd->ww) {
		if (!wd->ww->write(mem, length)) {
			wd->validation.error = true;
		}
	}
}

ROSE_INLINE void writedata_free(WriteData *wd) {
	if (wd->dna) {
		DNA_sdna_free(wd->dna);
	}

	MEM_freeN(wd);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Generic DNA File Writing
 * \{ */

ROSE_STATIC void writestruct_at_address_nr(WriteData *wd, int filecode, uint64_t struct_nr, int nr, const void *address, const void *data) {
	RHead head;

	if (address == NULL || data == NULL || nr == 0) {
		return;
	}

	head.filecode = filecode;
	head.size = nr * DNA_sdna_struct_size(wd->dna, struct_nr);
	head.length = nr;
	head.address = (uint64_t)address;
	head.dnatype = struct_nr;

	if (head.size == 0) {
		return;
	}

	writedata_do_write(wd, &head, sizeof(RHead));
	writedata_do_write(wd, data, head.size);
}

ROSE_STATIC void writestruct_nr(WriteData *wd, int filecode, uint64_t struct_nr, int nr, const void *address) {
	writestruct_at_address_nr(wd, filecode, struct_nr, nr, address, address);
}

ROSE_STATIC void writedata(WriteData *wd, int fildecode, size_t length, const void *address) {
	RHead head;

	if (address == NULL || length == 0) {
		return;
	}

	length = (length + 3) & ~(size_t)(3);

	if (length > INT_MAX) {
		ROSE_assert_msg(0, "Cannot write chunks bigger than INT_MAX!");
		return;
	}

	head.filecode = fildecode;
	head.size = length;
	head.length = 1;
	head.address = (uint64_t)address;
	head.dnatype = 0;

	writedata_do_write(wd, &head, sizeof(RHead));
	writedata_do_write(wd, address, head.size);
}

ROSE_STATIC void writelist_nr(WriteData *wd, int filecode, uint64_t struct_nr, const ListBase *lb) {
	const Link *link = (Link *)lb->first;

	while (link) {
		writestruct_nr(wd, filecode, struct_nr, 1, link);
		link = link->next;
	}
}

#define writestruct(wd, filecode, _struct, nr, data)                \
	do {                                                            \
		uint64_t struct_nr = DNA_sdna_struct_id(wd->dna, #_struct); \
		ROSE_assert(struct_nr != 0);                                \
		writestruct_nr(wd, filecode, struct_nr, nr, data);          \
	} while (false)

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Writing (Public)
 * \{ */

void RLO_write_struct_by_name(RoseWriter *writer, const char *struct_name, const void *data) {
	uint64_t struct_nr = DNA_sdna_struct_id(writer->wd->dna, struct_name);

	writestruct_nr(writer->wd, RLO_CODE_DATA, struct_nr, 1, data);
}

void RLO_write_raw(RoseWriter *writer, size_t size, const void *ptr) {
	writedata(writer->wd, RLO_CODE_DATA, size, ptr);
}

void RLO_write_char_array(struct RoseWriter *writer, size_t length, const char *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_int8_array(RoseWriter *writer, size_t length, const int8_t *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_uint8_array(RoseWriter *writer, size_t length, const uint8_t *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_int32_array(RoseWriter *writer, size_t length, const int32_t *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_uint32_array(RoseWriter *writer, size_t length, const uint32_t *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_float_array(struct RoseWriter *writer, size_t length, const float *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_double_array(struct RoseWriter *writer, size_t length, const double *ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_pointer_array(struct RoseWriter *writer, size_t length, const void **ptr) {
	RLO_write_raw(writer, length * sizeof(ptr[0]), ptr);
}

void RLO_write_string(struct RoseWriter *writer, const char *ptr) {
	RLO_write_raw(writer, LIB_strlen(ptr) + 1, ptr);
}

void rlo_write_id_struct(struct RoseWriter *writer, const char *struct_name, const void *id_address, const ID *id) {
	uint64_t struct_nr = DNA_sdna_struct_id(writer->wd->dna, struct_name);

	writestruct_at_address_nr(writer->wd, GS(id->name), struct_nr, 1, id_address, id);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name ID Writing (Private)
 * \{ */

/**
 * Specific code to prepare IDs to be written.
 *
 * Required for writing properly embedded IDs currently.
 *
 * \note Once there is a better generic handling of embedded IDs,
 * this may go back to private code in `writefile.cc`.
 */
struct RLO_Write_IDBuffer {
private:
	static constexpr int static_size = 8192;
	rose::DynamicStackBuffer<static_size> buffer_;

public:
	RLO_Write_IDBuffer(ID &id, bool is_placeholder);
	RLO_Write_IDBuffer(ID &id, RoseWriter *writer);

	ID *get() {
		return static_cast<ID *>(buffer_.buffer());
	};
};

RLO_Write_IDBuffer::RLO_Write_IDBuffer(ID &id, const bool is_placeholder) : buffer_(is_placeholder ? sizeof(ID) : KER_idtype_get_info_from_id(&id)->size, alignof(ID)) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_id(&id);
	ID *temp_id = static_cast<ID *>(buffer_.buffer());

	/* Copy ID data itself into buffer, to be able to freely modify it. */

	if (is_placeholder) {
		/* For placeholders (references to linked data), zero-initialize, and only explicitly copy the
		 * very small subset of required data. */
		*temp_id = ID{};
		temp_id->lib = id.lib;
		LIB_strcpy(temp_id->name, ARRAY_SIZE(temp_id->name), id.name);
		temp_id->flag = id.flag;
		temp_id->uuid = id.uuid;
		return;
	}

	/* Regular 'full' ID writing, copy everything, then clear some runtime data irrelevant in the
	 * blendfile. */
	memcpy(temp_id, &id, id_type->size);

	/* Clear runtime data to reduce false detection of changed data in undo/redo context. */
	temp_id->tag = 0;
	temp_id->user = 0;
	/**
	 * Those listbase data change every time we add/remove an ID, and also often when
	 * renaming one (due to re-sorting). This avoids generating a lot of false 'is changed'
	 * detections between undo steps.
	 */
	temp_id->prev = nullptr;
	temp_id->next = nullptr;
	/** 
	 * Those runtime pointers should never be set during writing stage, but just in case clear
	 * them too.
	 */
	temp_id->orig_id = nullptr;
	temp_id->newid = nullptr;
}

RLO_Write_IDBuffer::RLO_Write_IDBuffer(ID &id, RoseWriter *writer) : RLO_Write_IDBuffer(id, false) {
}

ROSE_STATIC void write_id(RoseWriter *writer, ID *id) {
	const IDTypeInfo *id_type = KER_idtype_get_info_from_id(id);

	if (id_type->write != nullptr) {
		RLO_Write_IDBuffer id_buffer{*id, false};
		id_type->write(writer, id_buffer.get(), id);
	}
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name File Writing (Private)
 * \{ */

ROSE_STATIC void write_dna(RoseWriter *writer, const SDNA *dna) {
	writedata(writer->wd, RLO_CODE_DNA1, dna->length, dna->data);
}

ROSE_STATIC void write_userdef(RoseWriter *writer, const UserDef *userdef) {
	writestruct(writer->wd, RLO_CODE_USER, UserDef, 1, userdef);

	LISTBASE_FOREACH(const Theme *, theme, &userdef->themes) {
		RLO_write_struct(writer, Theme, theme);
	}
}

ROSE_STATIC void write_libraries(RoseWriter *writer, Main *main) {
	ID *id;
	FOREACH_MAIN_ID_BEGIN(main, id) {
		write_id(writer, id);
	}
	FOREACH_MAIN_ID_END;
}

ROSE_STATIC void write_end(RoseWriter *writer) {
	RHead head;
	memset(&head, 0, sizeof(RHead));
	head.filecode = RLO_CODE_ENDB;
	writedata_do_write(writer->wd, &head, sizeof(RHead));
}

ROSE_STATIC bool write_file_handle(Main *main, WriteWrap *ww, int flag) {
	bool status;
	WriteData *wd = writedata_new(ww);
	RoseWriter writer = {wd};

	char header[8];
#ifdef __BIG_ENDIAN__
	LIB_strnformat(header, sizeof(header), "ROSEBG%c", '0' + sizeof(void *));
#else
	LIB_strnformat(header, sizeof(header), "ROSELT%c", '0' + sizeof(void *));
#endif
	writedata_do_write(wd, header, sizeof(header));

	write_dna(&writer, wd->dna);
	write_userdef(&writer, &U);
	write_libraries(&writer, main);
	write_end(&writer);

	status = !wd->validation.error;
	writedata_free(wd);
	return status;
}

/** \} */

bool RLO_write_file(Main *main, const char *filepath, int flag) {
	RawWriteWrap ww;
	if (!ww.open(filepath)) {
		return false;
	}
	if (!write_file_handle(main, &ww, flag)) {
		ww.close();
		return false;
	}
	if (!ww.close()) {
		return false;
	}
	return true;
}
