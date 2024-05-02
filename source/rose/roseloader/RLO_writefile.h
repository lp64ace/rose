#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ROSE_LOADER_WRITE_DATABLOCKS = (1 << 0),
};

bool RLO_write_file(struct Main *main, const char *filepath, int flags);

/* -------------------------------------------------------------------- */
/** \name Write Utils
 * \{ */

void RLO_write_id_struct(struct RoseWriter* writer, int struct_id, const void* id_address, const struct ID* id);

/**
 * Write raw data.
 */
void RLO_write_raw(struct RoseWriter* writer, size_t size_in_bytes, const void* data_ptr);
void RLO_write_int8_array(struct RoseWriter* writer, size_t num, const int8_t* data_ptr);
void RLO_write_int32_array(struct RoseWriter* writer, size_t num, const int32_t* data_ptr);
void RLO_write_uint32_array(struct RoseWriter* writer, size_t num, const uint32_t* data_ptr);
void RLO_write_float_array(struct RoseWriter* writer, size_t num, const float* data_ptr);
void RLO_write_double_array(struct RoseWriter* writer, size_t num, const double* data_ptr);
void RLO_write_float3_array(struct RoseWriter* writer, size_t num, const float* data_ptr);
void RLO_write_pointer_array(struct RoseWriter* writer, size_t num, const void* data_ptr);
/**
 * Write a null terminated string.
 */
void RLO_write_string(struct RoseWriter* writer, const char* data_ptr);

/* \} */

#ifdef __cplusplus
}
#endif
