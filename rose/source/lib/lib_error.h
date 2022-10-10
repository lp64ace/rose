#pragma once

#include <stdarg.h>
#include <stdbool.h>

#define ROSE_ERROR_OK			0x00
#define ROSE_ERROR_ASSERT		0xf1
#define ROSE_ERROR_UNREACHABLE		0xf2

#ifdef __cplusplus
extern "C" {
#endif

	void LIB_error ( int error , const char *file , int line , const char *func , const char *fmt , ... );
	void LIB_werror ( int error , const wchar_t *file , int line , const wchar_t *func , const wchar_t *fmt , ... );

	void LIB_verror ( int error , const char *file , int line , const char *func , const char *fmt , va_list args );
	void LIB_vwerror ( int error , const wchar_t *file , int line , const wchar_t *func , const wchar_t *fmt , va_list args );

	void LIB_throw_error ( int error );

#ifdef _DEBUG
#  define LIB_assert(expr)		if ( !!(expr) ) { } else { LIB_error(ROSE_ERROR_ASSERT,__FILE__,__LINE__,__func__,"%s",#expr); LIB_throw_error(ROSE_ERROR_ASSERT); }
#else
#  define LIB_assert(expr)		(void)0
#endif

#ifdef _DEBUG
#  define LIB_assert_msg(expr,fmt,...)	if ( !!(expr) ) { } else { LIB_error(ROSE_ERROR_ASSERT,__FILE__,__LINE__,__func__,fmt,__VA_ARGS__); LIB_throw_error(ROSE_ERROR_ASSERT); }
#else
#  define LIB_assert_msg(epxr,fmt,...)	if ( !!(expr) ) { } else { LIB_error(ROSE_ERROR_ASSERT,__FILE__,__LINE__,__func__,fmt,__VA_ARGS__); }
#endif

#ifdef _DEBUG
// Same as #LIB_assert_msg but does not throw
#  define LIB_assert_warn_msg(expr,fmt,...)	if ( !!(expr) ) { } else { LIB_error(ROSE_ERROR_ASSERT,__FILE__,__LINE__,__func__,fmt,__VA_ARGS__); }
#else
// Same as #LIB_assert_msg but does not throw
#  define LIB_assert_warn_msg(epxr,fmt,...)	if ( !!(expr) ) { } else { LIB_error(ROSE_ERROR_ASSERT,__FILE__,__LINE__,__func__,fmt,__VA_ARGS__); }
#endif

#define LIB_assert_unreachable()	LIB_error(ROSE_ERROR_UNREACHABLE,__FILE__,__LINE__,__func__,"Unreachable state"); LIB_throw_error(ROSE_ERROR_UNREACHABLE);

#ifdef __cplusplus
}
#endif
