#ifndef __MEMLEAK_H__
#define __MEMLEAK_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool leak_detector_has_run;
extern char free_after_leak_detection_message[];

#ifdef __cplusplus
}
#endif

#endif	// __MEMLEAK_H__
