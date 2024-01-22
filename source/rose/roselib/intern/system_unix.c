#include "LIB_system.h"

#if defined(__linux__) || defined(__unix__)

#	include <execinfo.h>

void LIB_system_backtrace(FILE *fp) {
#	define SIZE 100
	void *buffer[SIZE];
	int nptrs;
	char **strings;
	int i;

	nptrs = backtrace(buffer, SIZE);
	strings = backtrace_symbols(buffer, nptrs);
	for (i = 0; i < nptrs; i++) {
		fputs(strings[i], fp);
		fputc('\n', fp);
	}

	free(strings);
#	undef SIZE
}

#endif
