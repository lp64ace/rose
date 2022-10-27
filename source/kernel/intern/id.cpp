#include "id_private.h"

#include "lib/lib_error.h"

namespace rose {
namespace kernel {

ObjID::ObjID ( ) {
	this->mSource = NULL;
	this->mRefcount = 0;
}

ObjID::ObjID ( ObjID *source ) {
	this->mSource = source->GetSource ( );
	this->mRefcount = 0;

	// Mark source object as our dependency.
	this->AddDependency ( this->mSource );
}

ObjID::~ObjID ( ) {
	this->RemDependency ( this->mSource );
	if ( this->mRefcount ) {
		fprintf ( stderr , "Referenced object was deleted.\n" );
		LIB_assert_unreachable ( );
	}
}

void ObjID::AddDependency ( ObjID *id ) {
	if ( !id || id == this ) {
		return;
	}
#ifdef SAFE_OBJECT_REFERENCES
	std::set<ObjID *>::iterator itr = this->mDependencies.find ( id );
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

void ObjID::RemDependency ( ObjID *id ) {
	if ( !id || id == this ) {
		return;
	}
	if ( id->mRefcount ) {
#ifdef SAFE_OBJECT_REFERENCES
		std::set<ObjID *>::iterator itr = this->mDependencies.find ( id );
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

bool ObjID::IsReferenced ( ) const {
	return this->mRefcount != 0;
}

int ObjID::GetRefcount ( ) const {
	return this->mRefcount;
}

ObjID *ObjID::GetSource ( ) {
	return ( this->mSource ) ? this->mSource : this;
}

const ObjID *ObjID::GetSource ( ) const {
	return ( this->mSource ) ? this->mSource : this;
}

ObjID *ObjID::GetSourceEx ( ) {
	return ( this->mSource != this ) ? this->mSource : NULL;
}

const ObjID *ObjID::GetSourceEx ( ) const {
	return ( this->mSource != this ) ? this->mSource : NULL;
}

bool ObjID::IsInstance ( ) const {
	return this->mSource != NULL;
}

}
}

using namespace rose;
using namespace rose::kernel;

void ID_add_dependency ( ID *self , ID *dependency ) {
	unwrap ( self )->AddDependency ( unwrap ( dependency ) );
}

void ID_rem_dependency ( ID *self , ID *dependency ) {
	unwrap ( self )->RemDependency ( unwrap ( dependency ) );
}

int ID_refcount ( const ID *id ) {
	return unwrap ( id )->GetRefcount ( );
}

ID *ID_get_source_safe ( ID *id ) {
	return wrap ( unwrap ( id )->GetSource ( ) );
}

ID *ID_get_source ( ID *id ) {
	return wrap ( unwrap ( id )->GetSourceEx ( ) );
}
