#pragma once

#define __ENDIAN_WORD__ 'Rose'

#if __ENDIAN_WORD__ == 0x526F7365
#	define BIG_ENDIAN
#elif __ENDIAN_WORD__ == 0x65736F52
#	define LITTLE_ENDIAN
#else
#	define UNKOWN_ENDIAN
#endif
