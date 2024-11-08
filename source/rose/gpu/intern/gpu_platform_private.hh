#pragma once

#include "GPU_platform.h"

namespace rose::gpu {

class GPUPlatformGlobal {
public:
	bool initialized = false;

	DeviceType device;
	OperatingSystemType system;
	DriverType driver;
	SupportLevel support_level;

	char *vendor = nullptr;
	char *renderer = nullptr;
	char *version = nullptr;
	char *support_key = nullptr;
	char *gpu_name = nullptr;

	BackendType backend = GPU_BACKEND_NONE;
	ArchitectureType architecture = GPU_ARCHITECTURE_IMR;

public:
	void init(DeviceType device, OperatingSystemType system, SupportLevel support_level, BackendType backend, const char *vendor, const char *renderer, const char *version, ArchitectureType architecture);
	void clear();
};

extern GPUPlatformGlobal GPG;

}  // namespace rose::gpu
