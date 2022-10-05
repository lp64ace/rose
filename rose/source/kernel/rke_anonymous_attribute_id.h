#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct AnonymousAttributeID AnonymousAttributeID;

	AnonymousAttributeID *RKE_anonymous_attribute_id_new_weak ( const char *debug_name );
	AnonymousAttributeID *RKE_anonymous_attribute_id_new_strong ( const char *debug_name );

	bool RKE_anonymous_attribute_id_has_strong_references ( const AnonymousAttributeID *anonymous_id );
	void RKE_anonymous_attribute_id_increment_weak ( const AnonymousAttributeID *anonymous_id );
	void RKE_anonymous_attribute_id_increment_strong ( const AnonymousAttributeID *anonymous_id );
	void RKE_anonymous_attribute_id_decrement_weak ( const AnonymousAttributeID *anonymous_id );
	void RKE_anonymous_attribute_id_decrement_strong ( const AnonymousAttributeID *anonymous_id );

	const char *RKE_anonymous_attribute_id_debug_name ( const AnonymousAttributeID *anonymous_id );
	const char *RKE_anonymous_attribute_id_internal_name ( const AnonymousAttributeID *anonymous_id );

#ifdef __cplusplus
}
#endif
