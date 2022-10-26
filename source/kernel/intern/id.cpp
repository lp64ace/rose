#include "id.h"

#include "lib/lib_error.h"

namespace rose {
namespace kernel {

ID::ID ( ) {
	this->mSource = NULL;
	this->mRefcount = 0;
}

ID::ID ( ID *source ) {
	this->mSource = source->GetSource ( );
	this->mRefcount = 0;

	// Mark source object as our dependency.
	this->AddDependency ( this->mSource );
}

ID::~ID ( ) {
	this->RemDependency ( this->mSource );
	if ( this->mRefcount ) {
		fprintf ( stderr , "Referenced object was deleted.\n" );
		LIB_assert_unreachable ( );
	}
}

void ID::AddDependency ( ID *id ) {
	if ( !id || id == this ) {
		return;
	}
#ifdef SAFE_OBJECT_REFERENCES
	std::set<ID *>::iterator itr = this->mDependencies.find ( id );
	if ( itr == this->mDependencies.end ( ) ) {
		id->mRefcount++;
		this->mMutex.lock ( );
		this->mDependencies.insert ( id );
		this->mMutex.unlock ( );
	} else {
		LIB_assert_msg ( 0 , "Object is already referenced." );
	}
#else
	id->mRefcount++;
#endif
}

void ID::RemDependency ( ID *id ) {
	if ( !id || id == this ) {
		return;
	}
	if ( id->mRefcount ) {
#ifdef SAFE_OBJECT_REFERENCES
		std::set<ID *>::iterator itr = this->mDependencies.find ( id );
		if ( itr != this->mDependencies.end ( ) ) {
			id->mRefcount--;
			this->mMutex.lock ( );
			this->mDependencies.erase ( itr );
			this->mMutex.unlock ( );
		} else {
			LIB_assert_msg ( 0 , "Non-referenced object cannot be dereferenced." );
		}
#else
		id->mRefcount--;
#endif
	}
}

bool ID::IsReferenced ( ) const {
	return this->mRefcount != 0;
}

int ID::GetRefcount ( ) const {
	return this->mRefcount;
}

ID *ID::GetSource ( ) {
	return ( this->mSource ) ? this->mSource : this;
}

const ID *ID::GetSource ( ) const {
	return ( this->mSource ) ? this->mSource : this;
}

bool ID::IsInstance ( ) const {
	return this->mSource != NULL;
}

}
}
