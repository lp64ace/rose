#pragma once

#include "context_private.h"

#ifndef NDEBUG
#  define SAFE_OBJECT_REFERENCES
#  include <set>
#endif

#include <mutex>

namespace rose {
namespace kernel {

class ID {
public:
	// Creates a new object id.
	ID ( );

	// Creates an instance object id from a source object.
	ID ( ID *source );

	// Delete the object's id.
	~ID ( );

	/** Mark an object as a dependency of this. Marking \a id as a referenced object and 
	* therefore an object that can not be deleted.
	* \param id The object that we want to reference ( as our dependency ). */
	void AddDependency ( ID *id );

	/** Update the specified object's reference counter and mark is as no longer required 
	* by this object.
	* \param id The object that we want to dereference. */
	void RemDependency ( ID *id );

	// Returns \c True if the object with the specified id is referenced.
	bool IsReferenced ( ) const;

	// Returns the number of object that have referenced this id.
	int GetRefcount ( ) const;

	// If this object is an instace the source object is returned, otherwise this method returns 'self'.
	ID *GetSource ( );

	// If this object is an instace the source object is returned, otherwise this method returns 'self'.
	const ID *GetSource ( ) const;

	// Returns \c True if this is an instace.
	bool IsInstance ( ) const;
private:
	ID *mSource;
	int mRefcount;

	std::mutex mMutex;

#ifdef SAFE_OBJECT_REFERENCES
	std::set<ID *> mDependencies;
#endif
};

}
}
