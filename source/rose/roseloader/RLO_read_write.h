#pragma once

struct RoseWriter;

#ifdef __cplusplus
extern "C" {
#endif

void RLO_write_raw(RoseWriter *writer, size_t length, const void *data_ptr);
void RLO_write_int8_array(RoseWriter *writer, size_t length, const int8_t *data_ptr);
void RLO_write_int32_array(RoseWriter *writer, size_t length, const int32_t *data_ptr);
void RLO_write_uint32_array(RoseWriter *writer, size_t length, const uint32_t *data_ptr);
void RLO_write_float_array(RoseWriter *writer, size_t length, const float *data_ptr);
void RLO_write_double_array(RoseWriter *writer, size_t length, const double *data_ptr);
void RLO_write_pointer_array(RoseWriter *writer, size_t length, const void *data_ptr);
void RLO_write_string(RoseWriter *writer, const char *data_ptr);

#ifdef __cplusplus
}
#endif
