#pragma once

#include "lib/lib_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

	// Opaque type hiding rose::kernel::Error.
	typedef struct Error Error;

	#define ROSE_SEVERITY_INFO		0
	#define ROSE_SEVERITY_WARNING		1
	#define ROSE_SEVERITY_ERROR		2
	#define ROSE_SEVERITY_FATAL_ERROR	3

	#define ROSE_NO_ERROR			0x0000
	#define ROSE_BAD_ARG			0x0f01
	#define ROSE_BAD_RET			0x0f02
	#define ROSE_OUT_OF_BOUNDS		0x0f03

#ifdef __cplusplus
}
#endif
