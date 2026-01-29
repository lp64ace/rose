#ifndef LIB_SESSION_UUID_H
#define LIB_SESSION_UUID_H

#include "DNA_session_uuid_types.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Generate new UUID which is unique throughout the Blender session. */
SessionUUID LIB_session_uuid_generate(void);
/** Check whether the UUID is properly generated. */
bool LIB_session_uuid_is_generated(const SessionUUID *uuid);
/** Check whether two UUIDs are identical. */
bool LIB_session_uuid_is_equal(const SessionUUID *lhs, const SessionUUID *rhs);

uint64_t LIB_session_uuid_hash_uint64(const SessionUUID *uuid);

/* Utility functions to make it possible to create GHash/GSet with UUID as a key. */

unsigned int LIB_session_uuid_ghash_hash(const void *uuid_v);
bool LIB_session_uuid_ghash_compare(const void *lhs_v, const void *rhs_v);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

namespace rose {

inline const bool operator==(const SessionUUID &lhs, const SessionUUID &rhs) {
	return LIB_session_uuid_is_equal(&lhs, &rhs);
}

template<typename T> struct DefaultHash;

template<> struct DefaultHash<SessionUUID> {
	uint64_t operator()(const SessionUUID &value) const {
		return LIB_session_uuid_hash_uint64(&value);
	}
};

}  // namespace rose

#endif

#endif	// !LIB_SESSION_UUID_H
