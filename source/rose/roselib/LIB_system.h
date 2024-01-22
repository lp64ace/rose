#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This is called on DEBUG mode to show the callstack and other code-related information on the specified stream output.
 * \param log The file-stream output to write the information to.
 *
 * \note This function is defined in `system_win32.c` for Windows systems and `system_unix.c` for Unix systems.
 */
void LIB_system_backtrace(FILE *log);

/** This function is used for debugging purposes, it will set interrupts for signal handling. */
void LIB_system_signal_callbacks_init();

#ifdef __cplusplus
}
#endif
