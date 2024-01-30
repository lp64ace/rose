#pragma once

#define ENDIAN_WORD 'Rose'

#if ENDIAN_WORD == 0x526F7365
#	define BIG_ENDIAN
#elif ENDIAN_WORD == 0x65736F52
#	define LITTLE_ENDIAN
#else
#	define UNKOWN_ENDIAN
#endif
