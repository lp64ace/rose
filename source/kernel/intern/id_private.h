#pragma once

#include "id.h"

#include "context_private.h"

#ifndef NDEBUG
#  define SAFE_OBJECT_REFERENCES
#  include <set>
#endif

#include <mutex>

namespace rose {
namespace kernel {

class ObjID {
protected:
	// Creates a new object id.
	ObjID ( );

	// Creates an instance object id from a source object.
	ObjID ( ObjID *source );
public:
	// Delete the object's id.
	virtual ~ObjID ( );

	/** Mark an object as a dependency of this. Marking \a id as a referenced object and
	* therefore an object that can not be deleted.
	* \param id The object that we want to reference ( as our dependency ). */
	void AddDependency ( ObjID *id );

	/** Update the specified object's reference counter and mark is as no longer required
	* by this object.
	* \param id The object that we want to dereference. */
	void RemDependency ( ObjID *id );

	// Returns \c True if the object with the specified id is referenced.
	bool IsReferenced ( ) const;

	// Returns the number of object that have referenced this id.
	int GetRefcount ( ) const;

	/** If this object is an instace the source object is returned , otherwise this method returns 'self'.
	* The difference with #GetSourceEx is that this method never returns NULL. */
	ObjID *GetSource ( );

	/** If this object is an instace the source object is returned , otherwise this method returns 'self'.
	* The difference with #GetSourceEx is that this method never returns NULL. */
	const ObjID *GetSource ( ) const;

	// If this object is an instace the source object is returned, otherwise this method returns 'NULL'.
	ObjID *GetSourceEx ( );

	// If this object is an instace the source object is returned, otherwise this method returns 'NULL'.
	const ObjID *GetSourceEx ( ) const;

	// Returns \c True if this is an instace.
	bool IsInstance ( ) const;
private:
	void AddReference ( );
	void RemReference ( );
private:
	ObjID *mSource;

	std::mutex mMutex;

	int mRefcount;
#ifdef SAFE_OBJECT_REFERENCES
	std::set<ObjID *> mDependencies;
#endif
};

/* Syntactic sugar. */
static inline ID *wrap ( ObjID *vert ) {
	return reinterpret_cast< ID * >( vert );
}
static inline ObjID *unwrap ( ID *vert ) {
	return reinterpret_cast< ObjID * >( vert );
}
static inline const ObjID *unwrap ( const ID *vert ) {
	return reinterpret_cast< const ObjID * >( vert );
}

}
}
