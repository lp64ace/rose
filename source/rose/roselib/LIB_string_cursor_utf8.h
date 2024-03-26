#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool LIB_str_cursor_step_next_utf8(const char *str, int str_maxlen, int *pos);
bool LIB_str_cursor_step_prev_utf8(const char *str, int str_maxlen, int *pos);

#ifdef __cplusplus
}
#endif
