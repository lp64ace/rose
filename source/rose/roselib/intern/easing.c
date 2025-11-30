#include "LIB_easing.h"

float LIB_easing_linear_ease(float time, float begin, float change, float duration) {
	return change * time / duration + begin;
}
