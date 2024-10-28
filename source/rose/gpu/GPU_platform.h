#pragma once

#include "LIB_utildefines.h"

typedef enum BackendType {
	GPU_BACKEND_NONE = 0,
	GPU_BACKEND_OPENGL = 1 << 0,
	GPU_BACKEND_ANY = 0xff,
} BackendType;

typedef enum DeviceType {
	GPU_DEVICE_NVIDIA = 1 << 0,
	GPU_DEVICE_ATI = 1 << 1,
	GPU_DEVICE_INTEL = 1 << 2,
	GPU_DEVICE_INTEL_UHD = 1 << 3,
	GPU_DEVICE_APPLE = 1 << 4,
	GPU_DEVICE_SOFTWARE = 1 << 5,
	GPU_DEVICE_QUALCOMM = 1 << 6,
	GPU_DEVICE_UNKNOWN = 1 << 7,
	GPU_DEVICE_ANY = 0xff,
} DeviceType;

ENUM_OPERATORS(DeviceType, GPU_DEVICE_ANY);

typedef enum OperatingSystemType {
	GPU_OS_WIN = 1 << 8,
	GPU_OS_MAC = 1 << 9,
	GPU_OS_UNIX = 1 << 10,
	GPU_OS_ANY = 0xff00,
} OperatingSystemType;

ENUM_OPERATORS(OperatingSystemType, GPU_OS_ANY);

typedef enum DriverType {
	GPU_DRIVER_OFFICIAL = 1 << 16,
	GPU_DRIVER_OPENSOURCE = 1 << 17,
	GPU_DRIVER_SOFTWARE = 1 << 18,
	GPU_DRIVER_ANY = 0xff0000,
} DriverType;

ENUM_OPERATORS(DriverType, GPU_DRIVER_ANY);

typedef enum SupportLevel {
	GPU_SUPPORT_LEVEL_SUPPORTED,
	GPU_SUPPORT_LEVEL_LIMITED,
	GPU_SUPPORT_LEVEL_UNSUPPORTED,
} SupportLevel;

typedef enum ArchitectureType {
	/**
	 * Immediate Mode Renderer (IMR)
	 *
	 * Typically, an IMR architecture will execute GPU work in sequence, rasterizing primitives in order.
	 */
	GPU_ARCHITECTURE_IMR = 0,
	/**
	 * Tile-Based-Deferred-Renderer (TBDR)
	 *
	 * A TBDR architecture will typically execute the vertex stage up-front for all primitives,
	 * binning geometry into distinct tiled regions. Fragments will then be restarized within the bounds of one tile at a time.
	 */
	GPU_ARCHITECTURE_TBDR = 1,
} ArchitectureType;

#if defined(__cplusplus)
extern "C" {
#endif

bool GPU_type_matches(DeviceType device, OperatingSystemType system, DriverType driver);
bool GPU_type_matches_ex(DeviceType device, OperatingSystemType system, DriverType driver, BackendType backend);

SupportLevel GPU_platform_support_level(void);
ArchitectureType GPU_platform_architecture(void);

const char *GPU_platform_vendor(void);
const char *GPU_platform_renderer(void);
const char *GPU_platform_version(void);
const char *GPU_platform_support_level_key(void);
const char *GPU_platform_gpu_name(void);

#if defined(__cplusplus)
}
#endif
