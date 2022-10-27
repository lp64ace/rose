#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	// Opaque type hiding rose::kernel::ID.
	typedef struct ID ID;

	void ID_add_dependency ( ID *self , ID *dependency );
	void ID_rem_dependency ( ID *self , ID *dependency );

	int ID_refcount ( const ID *id );

	ID *ID_get_source_safe ( ID *id );

	ID *ID_get_source ( ID *id );

#ifdef __cplusplus
}
#endif