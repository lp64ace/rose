#include "GPU_info.h"

#include "gpu_info_private.hh"

/** An integer can hold both a float and an integer. */
static int s_info_buffer[GPU_INFO_LEN];

int &gpu_get_info_i(int info) {
	return *(((int *)s_info_buffer) + info);
}

float &gpu_get_info_f(int info) {
	return *(((float *)s_info_buffer) + info);
}

int &gpu_set_info_i(int info, int val) {
	return gpu_get_info_i(info) = val;
}

float &gpu_set_info_f(int info, float val) {
	return gpu_get_info_f(info) = val;
}

int GPU_get_info_i(int info) {
	return gpu_get_info_i(info);
}

float GPU_get_info_f(int info) {
	return gpu_get_info_f(info);
}
