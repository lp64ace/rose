#pragma once

#include "gpu/gpu_info.h"

struct GPU_PlatformGlobal {
	bool mInitialized = false;
	int mDevice;
	int mOperatingSystem;
	int mDriver;
	int mSupportLevel;
	char *mVendor = nullptr;
	char *mRenderer = nullptr;
	char *mVersion = nullptr;
	char *mSupportKey = nullptr;
	char *mName = nullptr;
	int mBackend = GPU_BACKEND_NONE;
	
	void Init ( int device , int os , int driver , int support , int backend , const char *vendor , const char *renderer , const char *version );

	void Clear ( );
};

void gpu_platform_init ( int device , int os , int driver , int support , int backend , const char *vendor , const char *renderer , const char *version );
void gpu_platform_clear ( );

int &gpu_set_info_i ( unsigned int info , int val );
float &gpu_set_info_f ( unsigned int info , float val );
int &gpu_get_info_i ( unsigned int info );
float &gpu_get_info_f ( unsigned int info );
