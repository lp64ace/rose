#include "LIB_time.h"

#include "glib.h"

float _check_seconds_timer_float(void) {
	return GHOST_GetTime32();
}

double _check_seconds_timer_double(void) {
	return GHOST_GetTime64();
}
