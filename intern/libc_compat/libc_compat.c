#ifdef __linux__
#	include <features.h>
#	include <math.h>
#	include <stdlib.h>
#	if defined(__GLIBC_PREREQ)
#		if __GLIBC_PREREQ(2, 34)

extern void *(*__malloc_hook)(size_t __size, const void *);
extern void *(*__realloc_hook)(void *__ptr, size_t __size, const void *);
extern void *(*__memalign_hook)(size_t __alignment, size_t __size, const void *);
extern void (*__free_hook)(void *__ptr, const void *);

void *(*__malloc_hook)(size_t __size, const void *) = NULL;
void *(*__realloc_hook)(void *__ptr, size_t __size, const void *) = NULL;
void *(*__memalign_hook)(size_t __alignment, size_t __size, const void *) = NULL;
void (*__free_hook)(void *__ptr, const void *) = NULL;

#		endif // __GLIBC_PREREQ(2, 34)
#	endif // defined(__GLIBC_PREREQ)
#endif // __linux__

