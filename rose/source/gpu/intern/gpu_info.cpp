#include "gpu_info_private.h"

#include "lib/lib_alloc.h"
#include "lib/lib_string_util.h"

#include <stdio.h>

static char *gpu_platform_create_key ( int support , const char *vendor , const char *renderer , const char *version ) {
	char *key = ( char * ) MEM_mallocN ( 1024 , __FUNCTION__ );
	if ( key ) {
		int len = sprintf_s ( key , 1024 , "[%s] %s %s" , vendor , renderer , version );
#ifdef TRUST_NO_ONE
		assert ( len < 1024 );
		key [ len ] = 0;
#endif
		switch ( support ) {
			case GPU_SUPPORT_LEVEL_SUPPORTED: strcat_s ( key , 1024 , "SUPPORTED" ); break;
			case GPU_SUPPORT_LEVEL_LIMITED: strcat_s ( key , 1024 , "LIMITED" ); break;
			default: strcat_s ( key , 1024 , "UNSUPPORTED" ); break;
		}
	}
	return key;
}

static char *gpu_platform_gpu_name ( const char *vendor , const char *renderer , const char *version ) {
	char *key = ( char * ) MEM_mallocN ( 1024 , __FUNCTION__ );
	if ( key ) {
		sprintf_s ( key , 1024 , "[%s] %s %s" , vendor , renderer , version );
	}
	return key;
}

void GPU_PlatformGlobal::Init ( int device , int os , int driver , int support , int backend , const char *_vendor , const char *_renderer , const char *_version ) {
	this->Clear ( );

	this->mInitialized = true;

	this->mDevice = device;
	this->mOperatingSystem = os;
	this->mDriver = driver;
	this->mSupportLevel = support;

	const char *vendor = _vendor ? _vendor : "UNKNOWN";
	const char *renderer = _renderer ? _renderer : "UNKNOWN";
	const char *version = _version ? _version : "UNKNOWN";

	this->mVendor = LIB_strdup ( vendor );
	this->mRenderer = LIB_strdup ( renderer );
	this->mVersion = LIB_strdup ( version );

	this->mSupportKey = gpu_platform_create_key ( support , vendor , renderer , version );
	this->mName = gpu_platform_gpu_name ( vendor , renderer , version );

	this->mBackend = backend;
}

void GPU_PlatformGlobal::Clear ( ) {
	MEM_SAFE_FREE ( this->mVendor );
	MEM_SAFE_FREE ( this->mRenderer );
	MEM_SAFE_FREE ( this->mVersion );
	MEM_SAFE_FREE ( this->mSupportKey );
	MEM_SAFE_FREE ( this->mName );
	this->mInitialized = false;
}

GPU_PlatformGlobal Platfom;

void gpu_platform_init ( int device , int os , int driver , int support , int backend , const char *vendor , const char *renderer , const char *version ) {
	Platfom.Init ( device , os , driver , support , backend , vendor , renderer , version );
}

void gpu_platform_clear ( ) {
	Platfom.Clear ( );
}

bool GPU_type_matches ( int device , int os , int driver ) {
	return GPU_type_matches_ex ( device , os , driver , GPU_BACKEND_ANY );
}

bool GPU_type_matches_ex ( int device , int os , int driver , int backend ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return ( Platfom.mDevice & device ) && ( Platfom.mOperatingSystem & os ) &&
		( Platfom.mDriver & driver ) && ( Platfom.mBackend & backend );
}

int GPU_platform_support_level ( void ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return Platfom.mSupportLevel;
}

const char *GPU_platform_vendor ( void ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return Platfom.mVendor;
}

const char *GPU_platform_renderer ( void ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return Platfom.mRenderer;
}

const char *GPU_platform_version ( void ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return Platfom.mVersion;
}

const char *GPU_platform_support_level_key ( void ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return Platfom.mSupportKey;
}

const char *GPU_platform_gpu_name ( void ) {
#ifdef TRUST_NO_ONE
	assert ( Platfom.mInitialized );
#endif
	return Platfom.mName;
}

//

int InfoBuffer [ GPU_INFO_LEN ];

int &gpu_get_info_i ( unsigned int info ) {
	return *( ( ( int * ) InfoBuffer ) + info );
}

float &gpu_get_info_f ( unsigned int info ) {
	return *( ( ( float * ) InfoBuffer ) + info );
}

int &gpu_set_info_i ( unsigned int info , int val ) {
	return gpu_get_info_i ( info ) = val;
}

float &gpu_set_info_f ( unsigned int info , float val ) {
	return gpu_get_info_f ( info ) = val;
}

int GPU_get_info_i ( unsigned int info ) {
	return gpu_get_info_i ( info );
}

float GPU_get_info_f ( unsigned int info ) {
	return gpu_get_info_f ( info );
}
