#include "MEM_guardedalloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"

#include "GPU_platform.h"

#include "gpu_platform_private.hh"

#include <stdio.h>

/* -------------------------------------------------------------------- */
/** \name Global GPU Platform
 * \{ */

namespace rose::gpu {

GPUPlatformGlobal GPG;

static char *create_key(SupportLevel support_level, const char *vendor, const char *renderer, const char *version) {
	char *support_key = NULL;

	switch (support_level) {
		case GPU_SUPPORT_LEVEL_SUPPORTED: {
			support_key = LIB_strnformat_allocN("[%s/SUPPORTED] %s %s", vendor, renderer, version);
		} break;
		case GPU_SUPPORT_LEVEL_LIMITED: {
			support_key = LIB_strnformat_allocN("[%s/LIMITED] %s %s", vendor, renderer, version);
		} break;
		default: {
			support_key = LIB_strnformat_allocN("[%s/UNSUPPORTED] %s %s", vendor, renderer, version);
		} break;
	}

	LIB_string_replace_single(support_key, '\n', ' ');
	LIB_string_replace_single(support_key, '\r', ' ');
	return support_key;
}

static char *create_gpu_name(const char *vendor, const char *renderer, const char *version) {
	char *gpu_name = LIB_strnformat_allocN("[%s] %s %s", vendor, renderer, version);

	LIB_string_replace_single(gpu_name, '\n', ' ');
	LIB_string_replace_single(gpu_name, '\r', ' ');
	return gpu_name;
}

void GPUPlatformGlobal::init(DeviceType device, OperatingSystemType system, SupportLevel support_level, BackendType backend, const char *_vendor, const char *_renderer, const char *_version, ArchitectureType architecture) {
	this->clear();

	this->initialized = true;

	this->device = device;
	this->system = system;
	this->support_level = support_level;
	this->backend = backend;

	const char *vendor = (_vendor) ? _vendor : "UNKOWN";
	const char *renderer = (_renderer) ? _renderer : "UNKOWN";
	const char *version = (_version) ? _version : "UNKOWN";

	this->vendor = LIB_strdupN(vendor);
	this->renderer = LIB_strdupN(renderer);
	this->version = LIB_strdupN(version);

	this->support_key = create_key(support_level, vendor, renderer, version);
	this->gpu_name = create_gpu_name(vendor, renderer, version);

	this->backend = backend;
	this->architecture = architecture;
}

void GPUPlatformGlobal::clear() {
	MEM_SAFE_FREE(vendor);
	MEM_SAFE_FREE(renderer);
	MEM_SAFE_FREE(version);
	MEM_SAFE_FREE(support_key);
	MEM_SAFE_FREE(gpu_name);

	this->initialized = false;
}

}  // namespace rose::gpu

/* \} */

/* -------------------------------------------------------------------- */
/** \name C-API
 * \{ */

using namespace rose::gpu;

SupportLevel GPU_platform_support_level() {
	ROSE_assert(GPG.initialized);
	return GPG.support_level;
}

const char *GPU_platform_vendor() {
	ROSE_assert(GPG.initialized);
	return GPG.vendor;
}

const char *GPU_platform_renderer() {
	ROSE_assert(GPG.initialized);
	return GPG.renderer;
}

const char *GPU_platform_version() {
	ROSE_assert(GPG.initialized);
	return GPG.version;
}

const char *GPU_platform_support_level_key() {
	ROSE_assert(GPG.initialized);
	return GPG.support_key;
}

const char *GPU_platform_gpu_name() {
	ROSE_assert(GPG.initialized);
	return GPG.gpu_name;
}

ArchitectureType GPU_platform_architecture() {
	ROSE_assert(GPG.initialized);
	return GPG.architecture;
}

bool GPU_type_matches(DeviceType device, OperatingSystemType system, DriverType driver) {
	return GPU_type_matches_ex(device, system, driver, GPU_BACKEND_ANY);
}

bool GPU_type_matches_ex(DeviceType device, OperatingSystemType system, DriverType driver, BackendType backend) {
	ROSE_assert(GPG.initialized);
	return (GPG.device & device) && (GPG.system & system) && (GPG.driver & driver) && (GPG.backend & backend);
}

/** \} */
