#pragma once

#include <atomic>
#include <string>

#include "lib/lib_hash.h"
#include "lib/lib_error.h"
#include "lib/lib_string_ref.h"

#include "rke_anonymous_attribute_id.h"

namespace rose {
namespace rke {

template<bool IsStrongReference> class OwnedAnonymousAttributeID {
private:
	const AnonymousAttributeID *mData = nullptr;

	template<bool OtherIsStrongReference> friend class OwnedAnonymousAttributeID;
public:
	OwnedAnonymousAttributeID ( ) = default;

	/** Create a new anonymous attribute id. */
	explicit OwnedAnonymousAttributeID ( StringRefNull debug_name ) {
		if constexpr ( IsStrongReference ) {
			this->mData = RKE_anonymous_attribute_id_new_strong ( debug_name.CStr ( ) );
		} else {
			this->mData = RKE_anonymous_attribute_id_new_weak ( debug_name.CStr ( ) );
		}
	}

	/**
	 * This transfers ownership, so no incref is necessary.
	 * The caller has to make sure that it owned the anonymous id.
	 */
	explicit OwnedAnonymousAttributeID ( const AnonymousAttributeID *anonymous_id ) : mData ( anonymous_id ) { }

	template<bool OtherIsStrong> OwnedAnonymousAttributeID ( const OwnedAnonymousAttributeID<OtherIsStrong> &other ) {
		this->mData = other.mData;
		this->incref ( );
	}

	template<bool OtherIsStrong> OwnedAnonymousAttributeID ( OwnedAnonymousAttributeID<OtherIsStrong> &&other ) {
		this->mData = other.mData;
		this->incref ( );
		other.decref ( );
		other.mData = nullptr;
	}

	~OwnedAnonymousAttributeID ( ) {
		this->decref ( );
	}

	template<bool OtherIsStrong> OwnedAnonymousAttributeID &operator=( const OwnedAnonymousAttributeID<OtherIsStrong> &other ) {
		if ( this == &other ) {
			return *this;
		}
		this->~OwnedAnonymousAttributeID ( );
		new ( this ) OwnedAnonymousAttributeID ( other );
		return *this;
	}

	template<bool OtherIsStrong> OwnedAnonymousAttributeID &operator=( OwnedAnonymousAttributeID<OtherIsStrong> &&other ) {
		if ( this == &other ) {
			return *this;
		}
		this->~OwnedAnonymousAttributeID ( );
		new ( this ) OwnedAnonymousAttributeID ( std::move ( other ) );
		return *this;
	}

	operator bool ( ) const {
		return this->mData != nullptr;
	}

	StringRefNull DebugName ( ) const {
		LIB_assert ( this->mData != nullptr );
		return RKE_anonymous_attribute_id_debug_name ( this->mData );
	}

	bool HasStrongReferences ( ) const {
		LIB_assert ( this->mData != nullptr );
		return RKE_anonymous_attribute_id_has_strong_references ( this->mData );
	}

	/** Extract the ownership of the currently wrapped anonymous id. */
	const AnonymousAttributeID *Extract ( ) {
		const AnonymousAttributeID *extracted_data = this->mData;
		/* Don't decref because the caller becomes the new owner. */
		this->mData = nullptr;
		return extracted_data;
	}

	/** Get the wrapped anonymous id, without taking ownership. */
	const AnonymousAttributeID *Get ( ) const {
		return this->mData;
	}
private:
	void incref ( ) {
		if ( this->mData == nullptr ) {
			return;
		}
		if constexpr ( IsStrongReference ) {
			RKE_anonymous_attribute_id_increment_strong ( this->mData );
		} else {
			RKE_anonymous_attribute_id_increment_weak ( this->mData );
		}
	}

	void decref ( ) {
		if ( this->mData == nullptr ) {
			return;
		}
		if constexpr ( IsStrongReference ) {
			RKE_anonymous_attribute_id_decrement_strong ( this->mData );
		} else {
			RKE_anonymous_attribute_id_decrement_weak ( this->mData );
		}
	}
};

using StrongAnonymousAttributeID = OwnedAnonymousAttributeID<true>;
using WeakAnonymousAttributeID = OwnedAnonymousAttributeID<false>;

}
}
