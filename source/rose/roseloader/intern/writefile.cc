#include "MEM_guardedalloc.h"

#include "DNA_sdna_types.h"
#include "DNA_userdef_types.h"

#include "LIB_assert.h"
#include "LIB_fileops.h"
#include "LIB_listbase.h"
#include "LIB_memory_utils.hh"
#include "LIB_set.hh"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_global.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"

#include "RLO_read_write.hh"
#include "RLO_writefile.h"

#include "RT_parser.h"

#include "intern/genfile.h"	 // DNA

#include <limits.h>
#include <zstd.h>

#define ZSTD_COMPRESSION_LEVEL 3

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

struct ZstdFrame {
	ZstdFrame *prev, *next;

	uint32_t compressed_size;
	uint32_t uncompressed_size;
};

class ZstdWriteWrap : public WriteWrap {
	struct ZstdWriteBlockTask;

public:
	ZstdWriteWrap(WriteWrap &base_wrap) : base_wrap(base_wrap) {
		LIB_listbase_clear(&this->tasks);
		LIB_listbase_clear(&this->threadpool);
		LIB_listbase_clear(&this->frames);
	}

	bool open(const char *filepath) override;
	bool close() override;
	bool write(const void *but, size_t lenth) override;

protected:
	void write_task(ZstdWriteBlockTask *task);
	void write_u32_le(uint32_t val);
	void write_seekable_frames();

private:
	WriteWrap &base_wrap;
	
	ListBase tasks;
	ListBase threadpool;
	ThreadMutex mutex;
	ThreadCondition condition;

	ListBase frames;
	size_t next_frame = 0;
	size_t num_frames = 0;

	bool write_error = false;
};

struct ZstdWriteWrap::ZstdWriteBlockTask {
	ZstdWriteBlockTask *prev, *next;

	void *data;
	size_t size;
	int frame_number;
	ZstdWriteWrap *ww;

	static void *write_task(void *userdata) {
		auto *task = static_cast<ZstdWriteBlockTask *>(userdata);
		task->ww->write_task(task);
		return nullptr;
	}
};

void ZstdWriteWrap::write_task(ZstdWriteBlockTask *task) {
	size_t out_buf_len = ZSTD_compressBound(task->size);
	void *out_buf = MEM_mallocN(out_buf_len, "Zstd out buffer");
	size_t out_size = ZSTD_compress(out_buf, out_buf_len, task->data, task->size, ZSTD_COMPRESSION_LEVEL);

	MEM_freeN(task->data);

	LIB_mutex_lock(&mutex);

	while (next_frame != task->frame_number) {
		LIB_condition_wait(&condition, &mutex);
	}

	if (ZSTD_isError(out_size)) {
		write_error = true;
	}
	else {
		if (base_wrap.write(out_buf, out_size)) {
			ZstdFrame *frameinfo = static_cast<ZstdFrame *>(MEM_mallocN(sizeof(ZstdFrame), "ZstdFrame"));
			frameinfo->uncompressed_size = task->size;
			frameinfo->compressed_size = out_size;
			LIB_addtail(&frames, frameinfo);
		}
		else {
			write_error = true;
		}
	}

	next_frame++;

	LIB_mutex_unlock(&mutex);
	LIB_condition_notify_all(&condition);

	MEM_freeN(out_buf);
}

void ZstdWriteWrap::write_u32_le(uint32_t val) {
	/* NOTE: this is endianness-sensitive. This value must always be written as little-endian. */
#ifndef __LITTLE_ENDIAN__
	ROSE_assert_unreachable();
#endif
	base_wrap.write(&val, sizeof(uint32_t));
}

/* In order to implement efficient seeking when reading the .rose, we append
 * a skippable frame that encodes information about the other frames present
 * in the file.
 * The format here follows the upstream spec for seekable files:
 * https://github.com/facebook/zstd/blob/master/contrib/seekable_format/zstd_seekable_compression_format.md
 * If this information is not present in a file (e.g. if it was compressed
 * with external tools), it can still be opened in Rose, but seeking will
 * not be supported, so more memory might be needed. */
void ZstdWriteWrap::write_seekable_frames() {
	/* Write seek table header (magic number and frame size). */
	write_u32_le(0x184D2A5E);

	/* The actual frame number might not match num_frames if there was a write error. */
	const size_t num_frames = LIB_listbase_count(&frames);
	/* Each frame consists of two u32, so 8 bytes each.
	 * After the frames, a footer containing two u32 and one byte (9 bytes total) is written. */
	const size_t frame_size = num_frames * 8 + 9;
	write_u32_le(frame_size);

	/* Write seek table entries. */
	LISTBASE_FOREACH(ZstdFrame *, frame, &this->frames) {
		write_u32_le(frame->compressed_size);
		write_u32_le(frame->uncompressed_size);
	}

	/* Write seek table footer (number of frames, option flags and second magic number). */
	write_u32_le(num_frames);
	const char flags = 0; /* We don't store checksums for each frame. */
	base_wrap.write(&flags, 1);
	write_u32_le(0x8F92EAB1);
}

bool ZstdWriteWrap::open(const char *filepath) {
	if (!this->base_wrap.open(filepath)) {
		return false;
	}

	/* Leave one thread open for the main writing logic, unless we only have one HW thread. */
	size_t num_threads = ROSE_MAX(1, LIB_system_thread_count() - 1);

	LIB_threadpool_init(&this->threadpool, ZstdWriteBlockTask::write_task, num_threads);
	LIB_mutex_init(&this->mutex);
	LIB_condition_init(&this->condition);

	return true;
}

bool ZstdWriteWrap::write(const void *buf, const size_t buf_len) {
	if (write_error) {
		return false;
	}

	ZstdWriteBlockTask *task = static_cast<ZstdWriteBlockTask *>(MEM_mallocN(sizeof(ZstdWriteBlockTask), "ZstdWriteBlockTask"));
	task->data = MEM_mallocN(buf_len, __func__);
	memcpy(task->data, buf, buf_len);
	task->size = buf_len;
	task->frame_number = num_frames++;
	task->ww = this;

	LIB_mutex_lock(&mutex);
	LIB_addtail(&tasks, task);

	/* If there's a free worker thread, just push the block into that thread.
	 * Otherwise, we wait for the earliest thread to finish.
	 * We look up the earliest thread while holding the mutex, but release it
	 * before joining the thread to prevent a deadlock. */
	ZstdWriteBlockTask *first_task = static_cast<ZstdWriteBlockTask *>(tasks.first);
	LIB_mutex_unlock(&mutex);
	if (!LIB_available_threads(&threadpool)) {
		LIB_threadpool_remove(&threadpool, first_task);

		/* If the task list was empty before we pushed our task, there should
		 * always be a free thread. */
		ROSE_assert(first_task != task);
		LIB_remlink(&tasks, first_task);
		MEM_freeN(first_task);
	}
	LIB_threadpool_insert(&threadpool, task);

	return true;
}

bool ZstdWriteWrap::close() {
	LIB_threadpool_end(&threadpool);
	LIB_freelistN(&tasks);

	LIB_mutex_end(&mutex);
	LIB_condition_end(&condition);

	write_seekable_frames();
	LIB_freelistN(&frames);

	return base_wrap.close() && !write_error;
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

	/**
	 * Keeps track of which shared data has been written for the current ID. This is necessary to
	 * avoid writing the same data more than once.
	 */
	rose::Set<const void *> per_id_written_shared_addresses;

	/** Wrap writing, so we can use zstd or other compression types later! */
	WriteWrap *ww;
} WriteData;

typedef struct RoseWriter {
	struct WriteData *wd;
} RoseWriter;

ROSE_INLINE WriteData *writedata_new(WriteWrap *ww) {
	WriteData *wd = MEM_new<WriteData>("WriteData");
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

	MEM_delete(wd);
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

	ROSE_assert(head.size);

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

void RLO_write_struct_by_name_at_address(struct RoseWriter *writer, const char *struct_name, const void *address, const void *data) {
	uint64_t struct_nr = DNA_sdna_struct_id(writer->wd->dna, struct_name);

	writestruct_at_address_nr(writer->wd, RLO_CODE_DATA, struct_nr, 1, address, data);
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

void RLO_write_struct_array_by_name(struct RoseWriter *writer, const char *struct_name, size_t length, const void *data) {
	uint64_t struct_nr = DNA_sdna_struct_id(writer->wd->dna, struct_name);

	writestruct_nr(writer->wd, RLO_CODE_DATA, struct_nr, length, data);
}

void RLO_write_struct_array_at_address_by_name(struct RoseWriter *writer, const char *struct_name, size_t length, const void *address, const void *data) {
	uint64_t struct_nr = DNA_sdna_struct_id(writer->wd->dna, struct_name);

	writestruct_at_address_nr(writer->wd, RLO_CODE_DATA, struct_nr, length, address, data);
}

void rlo_write_id_struct(struct RoseWriter *writer, const char *struct_name, const void *id_address, const ID *id) {
	uint64_t struct_nr = DNA_sdna_struct_id(writer->wd->dna, struct_name);

	writestruct_at_address_nr(writer->wd, GS(id->name), struct_nr, 1, id_address, id);
}

void RLO_write_shared_tag(struct RoseWriter *writer, const void *data) {
	if (data == NULL) {
		return;
	}

	// In case of UNDO we need to store the pointer to restore it later.
}

void RLO_write_shared(struct RoseWriter *writer, const void *data, size_t approximate_size_in_bytes, const ImplicitSharingInfoHandle *info, rose::FunctionRef<void()> write_fn) {
	if (data == NULL) {
		return;
	}
	if (info) {
		RLO_write_shared_tag(writer, data);
	}
	
	// TODO; handle UNDO here!

	if (info != NULL) {
		if (!writer->wd->per_id_written_shared_addresses.add(data)) {
			/* Was written already. */
			return;
		}
	}
	write_fn();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name ID Writing (Private)
 * \{ */

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

	writer->wd->per_id_written_shared_addresses.clear();
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
	RawWriteWrap raw_ww;

	ZstdWriteWrap ww(raw_ww);
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
