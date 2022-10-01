#pragma once

#include <stdarg.h>
#include <stdbool.h>

void LIB_error_log ( const char *fmt , ... );
void LIB_error_vlog ( const char *fmt , va_list args );

void LIB_assert_msg ( bool expr , const char *fmt , ... );
void LIB_assert_vmsg ( bool expr , const char *fmt , va_list args );

#ifdef _DEBUG
#  define LIB_assert(expr)		LIB_assert_msg(expr,"assertion failed '" #expr "' in %s::%d",__FILE__,__LINE__)
#else
#  define LIB_assert(expr)		(void)0
#endif
#define LIB_assert_unreachable()	LIB_error_log("Error in %s, this code should be unreachable.\n",__FUNCTION__)
