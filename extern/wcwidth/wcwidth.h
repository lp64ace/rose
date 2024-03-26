#ifndef __WCWIDTH_H__
#define __WCWIDTH_H__

#ifndef __cplusplus
#  if defined(__APPLE__) || defined(__NetBSD__) || defined(__OpenBSD__)
/* The <uchar.h> standard header is missing on macOS. */
#include <stddef.h>
typedef unsigned int char32_t;
#  else
#    include <uchar.h>
#  endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

int mk_wcwidth(char32_t ucs);
int mk_wcswidth(const char32_t *pwcs, size_t n);
int mk_wcwidth_cjk(char32_t ucs);
int mk_wcswidth_cjk(const char32_t *pwcs, size_t n);

#ifdef __cplusplus
}
#endif

#endif
