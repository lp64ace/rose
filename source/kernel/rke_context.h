#pragma once

#include "lib/lib_typedef.h"

#ifdef __cplusplus
extern "C" {
#endif

	#define RES_OK			0x0000 // The context member was found, and its data is available.
	#define RES_MEMBER_NOT_FOUND	0x0f01 // The context member was not found.
	#define RES_NO_DATA		0x0f02 // The context member was found, but its data is not available.

	// Opaque type hiding #rose::kernel::Context
	typedef struct rContext rContext;

	/** Allocates and returns a new rose context. 
	* \return Returns the newly activated context. */
	struct rContext *CTX_create ( );

	/** Discard the context, if the context was active the active context is set to 
	* an invalid context and then the context will be destroyed.
	* \param context The context that we want to destroy. */
	void CTX_free ( struct rContext *context );

#ifdef __cplusplus
}
#endif
