#ifndef RLO_READ_WRITE_H
#define RLO_READ_WRITE_H

#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID;
struct RoseDataReader;
struct RoseWriter;
struct UserDef;

#ifdef __BIG_ENDIAN__
/* Big Endian */
#	define MAKE_ID4(a, b, c, d) ((int)(a) << 24 | (int)(b) << 16 | (int)(c) << 8 | (int)(d))
#else
/* Little Endian */
#	define MAKE_ID4(a, b, c, d) ((int)(d) << 24 | (int)(c) << 16 | (int)(b) << 8 | (int)(a))
#endif

enum {
	/** Arbitrary allocated memory, usually owned by #ID's, will be freed when there are no users. */
	RLO_CODE_DATA = MAKE_ID4('D', 'A', 'T', 'A'),
	/** Used for storing the encoded SDNA string. */
	RLO_CODE_DNA1 = MAKE_ID4('D', 'N', 'A', '1'),
	/** Used for #UserDef, (user-preferences data). */
	RLO_CODE_USER = MAKE_ID4('U', 'S', 'E', 'R'),
	/** Terminate reading (no data). */
	RLO_CODE_ENDB = MAKE_ID4('E', 'N', 'D', 'B'),
};

/* -------------------------------------------------------------------- */
/** \name Rose Write API
 * \{ */

void RLO_write_struct_by_name(struct RoseWriter *writer, const char *name, const void *ptr);

void RLO_write_raw(struct RoseWriter *writer, size_t size, const void *ptr);

void RLO_write_char_array(struct RoseWriter *writer, size_t length, const char *ptr);
void RLO_write_int8_array(struct RoseWriter *writer, size_t length, const int8_t *ptr);
void RLO_write_uint8_array(struct RoseWriter *writer, size_t length, const uint8_t *ptr);
void RLO_write_int32_array(struct RoseWriter *writer, size_t length, const int32_t *ptr);
void RLO_write_uint32_array(struct RoseWriter *writer, size_t length, const uint32_t *ptr);
void RLO_write_float_array(struct RoseWriter *writer, size_t length, const float *ptr);
void RLO_write_double_array(struct RoseWriter *writer, size_t length, const double *ptr);
void RLO_write_pointer_array(struct RoseWriter *writer, size_t length, const void **ptr);
/** Write a null termianted string. */
void RLO_write_string(struct RoseWriter *writer, const char *ptr);

#define RLO_write_struct(writer, _struct, data)                            \
	do {                                                                   \
		RLO_write_struct_by_name(writer, #_struct, (const _struct *)data); \
	} while (false)

void rlo_write_id_struct(struct RoseWriter *writer, const char *name, const void *id_address, const struct ID *id);

#define RLO_write_id_struct(writer, _struct, id_address, id)   \
	do {                                                       \
		rlo_write_id_struct(writer, #_struct, id_address, id); \
	} while (false)

/** \} */

/* -------------------------------------------------------------------- */
/** \name Rose Read API
 * \{ */

typedef struct RoseDataReader RoseDataReader;

void *RLO_read_get_new_data_address(struct RoseDataReader *reader, const void *old_address);
void *RLO_read_get_new_data_address_no_user(struct RoseDataReader *reader, const void *old_address);
/** Reads the new data that are allocated for that address and replaces the old pointer with the new! */
#define RLO_read_data_address(reader, ptr_p) *((void **)ptr_p) = RLO_read_get_new_data_address((reader), *(ptr_p))

void *RLO_read_struct_array_with_size(struct RoseDataReader *reader, const void *old_address, size_t size);
#define RLO_read_struct(reader, struct_name, ptr_p) *((void **)ptr_p) = RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(struct_name))
#define RLO_read_struct_array(reader, struct_name, array_size, ptr_p) *((void **)ptr_p) = RLO_read_struct_array_with_size(reader, *((void **)ptr_p), sizeof(struct_name) * (array_size))

void RLO_read_struct_list_with_size(struct RoseDataReader *reader, size_t esize, ListBase *list);
#define RLO_read_struct_list(reader, struct_name, list) RLO_read_struct_list_with_size(reader, sizeof(struct_name), list)

/* Update data pointers and correct byte-order if necessary. */

void RLO_read_char_array(RoseDataReader *reader, int array_size, char **ptr_p);
void RLO_read_int8_array(RoseDataReader *reader, int array_size, int8_t **ptr_p);
void RLO_read_uint8_array(RoseDataReader *reader, int array_size, uint8_t **ptr_p);
void RLO_read_int32_array(RoseDataReader *reader, int array_size, int32_t **ptr_p);
void RLO_read_uint32_array(RoseDataReader *reader, int array_size, uint32_t **ptr_p);
void RLO_read_float_array(RoseDataReader *reader, int array_size, float **ptr_p);
void RLO_read_double_array(RoseDataReader *reader, int array_size, double **ptr_p);
void RLO_read_pointer_array(RoseDataReader *reader, int array_size, void **ptr_p);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Versioning API
 * \{ */

void rlo_do_versions_userdef(struct UserDef *user);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// RLO_READ_WRITE_H
