#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	// Opaque type hiding rose::kernel::ObjID.
	typedef struct ID ID;

	/** Mark an object as a dependency of this. Marking \a id as a referenced object and
	* therefore an object that can not be deleted.
	* \param self This object.
	* \param dependency The object that we want to reference ( as our dependency ). */
	void ID_add_dependency ( ID *self , ID *dependency );

	/** Update the specified object's reference counter and mark is as no longer required
	* by this object.
	* \param self This object.
	* \param dependency The object that we want to dereference. */
	void ID_rem_dependency ( ID *self , ID *dependency );

	// Returns the number of object that have referenced this id.
	int ID_refcount ( const ID *id );

	/** If this object is an instace the source object is returned , otherwise this method returns 'self'.
	* The difference with #ID_get_source is that this method never returns NULL. */
	ID *ID_get_source_safe ( ID *id );

	// If this object is an instace the source object is returned, otherwise this method returns 'NULL'.
	ID *ID_get_source ( ID *id );

#ifdef __cplusplus
}
#endif