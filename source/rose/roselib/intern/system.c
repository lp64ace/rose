#include <signal.h>

#include "LIB_system.h"

/**
 * Some function may be missing due to OS dependencies, see `system_*.c` files for functions that rely on operating system
 * functions like `RLI_system_backtrace()`.
 */

/* -------------------------------------------------------------------- */
/** \name Signal Interrupt
 * \{ */

static const char *lib_signal_translate(int signal) {
	switch(signal) {
		case SIGABRT: return "Abort";
		case SIGFPE: return "Floating-Point Error";
		case SIGILL: return "Illegal Instruction";
		case SIGINT: return "CTRL+C";
		case SIGSEGV: return "Illegal Storage Access";
		case SIGTERM: return "Termination Request";
	}
	return "Unkown Signal";
}
 
static void lib_signal_interup_info(int signal) {
	fprintf(stderr, "\n\n*** %s(%s) ***\n", __func__, lib_signal_translate(signal));
	LIB_system_backtrace(stderr);
}
 
static void lib_signal_interup_error(int signal) {
	fprintf(stderr, "\n\n*** %s(%s) ***\n", __func__, lib_signal_translate(signal));
	LIB_system_backtrace(stderr);
}

void LIB_system_signal_callbacks_init() {
	signal(SIGABRT, lib_signal_interup_error);
	signal(SIGFPE, lib_signal_interup_error);
	signal(SIGILL, lib_signal_interup_error);
	signal(SIGSEGV, lib_signal_interup_error);
	
	signal(SIGINT, lib_signal_interup_info);
	signal(SIGTERM, lib_signal_interup_info);
}

/* \} */
