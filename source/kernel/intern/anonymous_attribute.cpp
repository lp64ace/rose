#include "kernel/rke_anonymous_attribute.h"
#include "lib/lib_error.h"

using namespace rose::rke;

struct AnonymousAttributeID {
	/**
	 * Total number of references to this attribute id. Once this reaches zero, the struct can be
	 * freed. This includes strong and weak references.
	 */
	mutable std::atomic<int> refcount_tot = 0;

	/**
	 * Number of strong references to this attribute id. When this is zero, the corresponding
	 * attributes can be removed from geometries automatically.
	 */
	mutable std::atomic<int> refcount_strong = 0;

	/**
	 * Only used to identify this struct in a debugging session.
	 */
	std::string debug_name;

	/**
	 * Unique name of the this attribute id during the current session.
	 */
	std::string internal_name;
};

// Every time this function is called, it outputs a different name.
static std::string get_new_internal_name ( ) {
	static std::atomic<int> index = 0;
	const int next_index = index.fetch_add ( 1 );
	return ".a_" + std::to_string ( next_index );
}

AnonymousAttributeID *RKE_anonymous_attribute_id_new_weak ( const char *debug_name ) {
	AnonymousAttributeID *anonymous_id = new AnonymousAttributeID ( );
	anonymous_id->debug_name = debug_name;
	anonymous_id->internal_name = get_new_internal_name ( );
	anonymous_id->refcount_tot.store ( 1 );
	return anonymous_id;
}

AnonymousAttributeID *RKE_anonymous_attribute_id_new_strong ( const char *debug_name ) {
	AnonymousAttributeID *anonymous_id = new AnonymousAttributeID ( );
	anonymous_id->debug_name = debug_name;
	anonymous_id->internal_name = get_new_internal_name ( );
	anonymous_id->refcount_tot.store ( 1 );
	anonymous_id->refcount_strong.store ( 1 );
	return anonymous_id;
}

bool RKE_anonymous_attribute_id_has_strong_references ( const AnonymousAttributeID *anonymous_id ) {
	return anonymous_id->refcount_strong.load ( ) >= 1;
}

void RKE_anonymous_attribute_id_increment_weak ( const AnonymousAttributeID *anonymous_id ) {
	anonymous_id->refcount_tot.fetch_add ( 1 );
}

void RKE_anonymous_attribute_id_increment_strong ( const AnonymousAttributeID *anonymous_id ) {
	anonymous_id->refcount_tot.fetch_add ( 1 );
	anonymous_id->refcount_strong.fetch_add ( 1 );
}

void RKE_anonymous_attribute_id_decrement_weak ( const AnonymousAttributeID *anonymous_id ) {
	const int new_refcount = anonymous_id->refcount_tot.fetch_sub ( 1 ) - 1;
	if ( new_refcount == 0 ) {
		LIB_assert ( anonymous_id->refcount_strong == 0 );
		delete anonymous_id;
	}
}

void RKE_anonymous_attribute_id_decrement_strong ( const AnonymousAttributeID *anonymous_id ) {
	anonymous_id->refcount_strong.fetch_sub ( 1 );
	RKE_anonymous_attribute_id_decrement_weak ( anonymous_id );
}

const char *RKE_anonymous_attribute_id_debug_name ( const AnonymousAttributeID *anonymous_id ) {
	return anonymous_id->debug_name.c_str ( );
}

const char *RKE_anonymous_attribute_id_internal_name ( const AnonymousAttributeID *anonymous_id ) {
	return anonymous_id->internal_name.c_str ( );
}
