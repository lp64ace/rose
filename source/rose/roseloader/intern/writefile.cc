#include "DNA_listbase.h"
#include "DNA_ID.h"
#include "DNA_sdna_types.h"

#include "dna_genfile.h"
#include "dna_utils.h"

#include "RLO_rose_defs.h"
#include "RLO_writefile.h"

#include "LIB_assert.h"
#include "LIB_fileops.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

#include "KER_idtype.h"
#include "KER_version.h"
#include "KER_main.h"

#include "MEM_alloc.h"

#include <stdio.h>
#include <string.h>

/* Use optimal allocation since blocks of this size are kept in memory for undo. */
#define MEM_BUFFER_SIZE MEM_SIZE_OPTIMAL(1 << 17) /* 128kb */
#define MEM_CHUNK_SIZE MEM_SIZE_OPTIMAL(1 << 15)  /* ~32kb */

#define ZSTD_BUFFER_SIZE (1 << 21) /* 2mb */
#define ZSTD_CHUNK_SIZE (1 << 20)  /* 1mb */

/* -------------------------------------------------------------------- */
/** \name Internal Write Wrapper's (Abstracts Compression)
 * \{ */

class WriteWrap {
public:
	virtual bool open(const char* filepath) = 0;
	virtual bool close() = 0;
	virtual bool write(const void* buf, size_t buf_len) = 0;

	/** Buffer output (we only want when output isn't already buffered). */
	bool use_buf = true;
};

class RawWriteWrap : public WriteWrap {
public:
	bool open(const char* filepath) override;
	bool close() override;
	bool write(const void* buf, size_t buf_len) override;

private:
	FILE* file_handle = 0;
};

bool RawWriteWrap::open(const char* filepath) {
	if ((file_handle = LIB_fopen(filepath, LIB_FMODE_WRITE LIB_XMODE_BINARY)) == NULL) {
		return false;
	}
	return true;
}

bool RawWriteWrap::close() {
	if (file_handle) {
		LIB_fclose(file_handle);
		return true;
	}
	return false;
}

bool RawWriteWrap::write(const void* buf, size_t buf_len) {
	if (::fwrite(buf, buf_len, (size_t)1, file_handle) != (size_t)1) {
		return false;
	}
	return true;
}

/* \} */

using DefaultWriteWrap = RawWriteWrap;

/* -------------------------------------------------------------------- */
/** \name Write Data Type & Functions
 * \{ */

struct WriteData {
	const SDNA* sdna;

	/** Set on unlikely case of an error (ignores further file writing). */
	bool error;

	/**
	 * Wrap writting, so we can use zstd or other compression types later.
	 */
	WriteWrap* ww;
};

struct RoseWriter {
	WriteData* wd;
};

static WriteData* writedata_new(WriteWrap* ww) {
	WriteData* wd = static_cast<WriteData*>(MEM_callocN(sizeof(*wd), "WriteData"));
	wd->sdna = DNA_sdna_current_get();
	wd->ww = ww;
	return wd;
}

static void writedata_do_write(WriteData* wd, const void* mem, size_t memlen) {
	if ((wd == nullptr) || wd->error || (mem == nullptr) || memlen < 1) {
		return;
	}

	if (memlen > INT_MAX) {
		ROSE_assert_msg(0, "Cannot write chunks bigger than INT_MAX.");
		return;
	}

	if (!wd->ww->write(mem, memlen)) {
		wd->error = true;
	}
}

static void writedata_free(WriteData* wd) {
	MEM_freeN(wd);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Generic DNA File Writing
 * \{ */

 /**
  * Write a compiled structure using #WriteData.
  * \param wd The write data to use in order to write the structure.
  * \param filecode The type of data we are writing, see RLO_CODE_* enumerators.
  * \param struct_nr The index of the struct we are writing in the current #SDNA.
  * \param nr The number of structures in case this is an array.
  * \param adr The address of the memory the structure was stored in.
  * \param data The pointer to the structure data.
  */
static void writestruct_at_address_nr(WriteData* wd, int filecode, const int struct_nr, int nr, const void* adr, const void* data) {
	RHead head;

	ROSE_assert(0 <= struct_nr && struct_nr < wd->sdna->types_len);

	if (adr == NULL || data == NULL || nr == 0) {
		return;
	}

	head.code = filecode;
	head.address = adr;
	head.nr = nr;

	head.struct_nr = struct_nr;
	const DNAStruct* struct_info = &wd->sdna->types[head.struct_nr];

	head.length = nr * struct_info->size;

	if (head.length == 0) {
		return;
	}

	writedata_do_write(wd, &head, sizeof(RHead));
	writedata_do_write(wd, data, (size_t)head.length);
}

static void writestruct_nr(WriteData* wd, int filecode, const int struct_nr, int nr, const void* adr) {
	writestruct_at_address_nr(wd, filecode, struct_nr, nr, adr, adr);
}

static void writedata(WriteData* wd, int filecode, size_t len, const void* adr) {
	RHead head;

	if (adr == NULL || len == 0) {
		return;
	}

	/* Align to 4 bytes (writes uninitialized bytes in some cases). */
	len = (len + 3) & ~size_t(3);

	if (len > INT_MAX) {
		ROSE_assert_msg(0, "Cannot write chunks bigger than INT_MAX.");
		return;
	}

	head.code = filecode;
	head.address = adr;
	head.nr = 1;

	head.struct_nr = -1;
	head.length = (int)len;

	writedata_do_write(wd, &head, sizeof(RHead));
	writedata_do_write(wd, adr, (size_t)head.length);
}

static void writelist_nr(WriteData* wd, int filecode, const int struct_nr, const ListBase* lb) {
	const Link* link = static_cast<Link*>(lb->first);

	while (link) {
		writestruct_nr(wd, filecode, struct_nr, 1, link);
		link = link->next;
	}
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name File Writing (Private)
 * \{ */

#define ID_BUFFER_STATIC_SIZE 8192

typedef struct RLO_Write_IDBuffer {
	const IDTypeInfo* id_type;
	ID* temp_id;
	char id_buffer_static[ID_BUFFER_STATIC_SIZE];
} RLO_Write_IDBuffer;

static void id_buffer_init_for_id_type(RLO_Write_IDBuffer* id_buffer, const IDTypeInfo* id_type) {
	if (id_type != id_buffer->id_type) {
		const size_t idtype_struct_size = id_type->struct_size;
		if (idtype_struct_size > ID_BUFFER_STATIC_SIZE) {
			fprintf(stderr, "ID maximum buffer size (%d bytes) is not big enough to fit IDs of type %s, which needs %zu bytes", ID_BUFFER_STATIC_SIZE, id_type->name, idtype_struct_size);
			id_buffer->temp_id = static_cast<ID*>(MEM_mallocN(idtype_struct_size, __func__));
		}
		else {
			if (static_cast<void*>(id_buffer->temp_id) != id_buffer->id_buffer_static) {
				MEM_SAFE_FREE(id_buffer->temp_id);
			}
			id_buffer->temp_id = reinterpret_cast<ID*>(id_buffer->id_buffer_static);
		}
		id_buffer->id_type = id_type;
	}
}

static void id_buffer_init_from_id(RLO_Write_IDBuffer* id_buffer, ID* id) {
	ROSE_assert(id_buffer->id_type == KER_idtype_get_info_from_id(id));

	const size_t idtype_struct_size = id_buffer->id_type->struct_size;
	ID* temp_id = id_buffer->temp_id;
	memcpy(temp_id, id, idtype_struct_size);

	temp_id->prev = temp_id->next = NULL;
	temp_id->tag = 0;
	temp_id->users = 0;

	temp_id->newid = NULL;
	temp_id->orig_id = NULL;
}

static void id_buffer_free(RLO_Write_IDBuffer* id_buffer) {
	if (static_cast<void*>(id_buffer->temp_id) != id_buffer->id_buffer_static) {
		MEM_SAFE_FREE(id_buffer->temp_id);
	}
	MEM_delete<RLO_Write_IDBuffer>(id_buffer);
}

/**
 * Using the DNA module we could write every single data that we have into file, 
 * but that would be overkill since many data from ID data-blocks are not needed for the 
 * proper functionality of the system.
 * 
 * Some data can be automatically generated (like cache data or lookup data) etc.
 */

static bool write_file_handle(struct Main* main, struct WriteWrap* ww) {
	char buf[16];

	WriteData* wd;

	wd = writedata_new(ww);
	RoseWriter writer = { wd };

	LIB_snprintf(buf, ARRAY_SIZE(buf), "ROSE%c.3s", ((sizeof(void*) == 8) ? 'X' : 'x'), ROSE_VERSION);

	/** First we write the version of the application. */
	writedata_do_write(wd, buf, 9);
	
	/** Then we write the SDNA data so that we know information about structures. */
	const struct SDNA* sdna = wd->sdna;
	writedata(wd, RLO_CODE_DNA1, sdna->data_len, sdna->data);

	/** Then we write information about the data blocks in the #Main database. */
	do {
		ListBase* lbarray[INDEX_ID_MAX];
		int a = set_listbasepointers(main, lbarray);

		RLO_Write_IDBuffer* id_buffer = MEM_cnew<RLO_Write_IDBuffer>(__func__);

		while (a--) {
			ID* id = reinterpret_cast<ID*>(lbarray[a]->first);

			if (id == nullptr || GS(id->name) == ID_LI) {
				/** Do not write libraries in the file. */
				continue;
			}

			const IDTypeInfo* id_type = KER_idtype_get_info_from_id(id);

			id_buffer_init_for_id_type(id_buffer, id_type);

			for (; id; id = static_cast<ID*>(id->next)) {
				/**
				 * How did we even get here? ID data-blocks that are tagged as #LIB_TAG_NO_MAIN or #LIB_TAG_NO_USER_REFCOUNT
				 * should not be in a #Main database.
				 */
				ROSE_assert((id->tag & (LIB_TAG_NO_MAIN | LIB_TAG_NO_USER_REFCOUNT)) == 0);

				id_buffer_init_from_id(id_buffer, id);

				if (id_type->rose_write != nullptr) {
					id_type->rose_write(&writer, static_cast<ID*>(id_buffer->temp_id), id);
				}
			}
		}

		id_buffer_free(id_buffer);
	} while (false);

	return true;
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Write #Main database in file
 * \{ */

static bool rlo_write_file_impl(struct Main* main, const char* filepath, int flags, struct WriteWrap* ww) {
	if (ww->open(filepath) == false) {
		fprintf(stderr, "Cannot open file %s for writting: %s", filepath, strerror(errno));
		return false;
	}

	if (write_file_handle(main, ww)) {
		return false;
	}

	if (ww->close() == false) {
		fprintf(stderr, "Cannot close file %s reason: %s", filepath, strerror(errno));
		return false;
	}

	return true;
}

bool RLO_write_file(struct Main* main, const char* filepath, int flags) {
	DefaultWriteWrap wrap;

	return rlo_write_file_impl(main, filepath, flags, &wrap);
}

/* \} */

/* -------------------------------------------------------------------- */
/** \name Write Utils
 * \{ */

void RLO_write_id_struct(RoseWriter* writer, int struct_id, const void* id_address, const ID* id) {
	writestruct_at_address_nr(writer->wd, GS(id->name), struct_id, 1, id_address, id);
}

void RLO_write_raw(RoseWriter* writer, size_t size_in_bytes, const void* data_ptr) {
	writedata(writer->wd, RLO_CODE_DATA, size_in_bytes, data_ptr);
}

void RLO_write_int8_array(RoseWriter* writer, size_t num, const int8_t* data_ptr) {
	RLO_write_raw(writer, sizeof(int8_t) * num, data_ptr);
}
void RLO_write_int32_array(RoseWriter* writer, size_t num, const int32_t* data_ptr) {
	RLO_write_raw(writer, sizeof(int32_t) * num, data_ptr);
}
void RLO_write_uint32_array(RoseWriter* writer, size_t num, const uint32_t* data_ptr) {
	RLO_write_raw(writer, sizeof(uint32_t) * num, data_ptr);
}
void RLO_write_float_array(RoseWriter* writer, size_t num, const float* data_ptr) {
	RLO_write_raw(writer, sizeof(float) * num, data_ptr);
}
void RLO_write_double_array(RoseWriter* writer, size_t num, const double* data_ptr) {
	RLO_write_raw(writer, sizeof(double) * num, data_ptr);
}
void RLO_write_float3_array(RoseWriter* writer, size_t num, const float* data_ptr) {
	RLO_write_raw(writer, sizeof(float[3]) * num, data_ptr);
}
void RLO_write_pointer_array(RoseWriter* writer, size_t num, const void* data_ptr) {
	RLO_write_raw(writer, sizeof(void *) * num, data_ptr);
}
void RLO_write_string(RoseWriter* writer, const char* data_ptr) {
	if (data_ptr != NULL) {
		RLO_write_raw(writer, LIB_strlen(data_ptr) + 1, data_ptr);
	}
}

/* \} */
