#include "LIB_session_uuid.h"
#include "LIB_utildefines.h"

#include "atomic_ops.h"

/* Special value which indicates the UUID has not been assigned yet. */
#define ROSE_SESSION_UUID_NONE 0

static const SessionUUID global_session_uuid_none = {ROSE_SESSION_UUID_NONE};
static SessionUUID global_session_uuid = {ROSE_SESSION_UUID_NONE};

SessionUUID LIB_session_uuid_generate(void) {
	SessionUUID result;
	result.uuid_ = atomic_add_and_fetch_uint64(&global_session_uuid.uuid_, 1);
	if (!LIB_session_uuid_is_generated(&result)) {
		/* Happens when the UUID overflows.
		 *
		 * Just request the UUID once again, hoping that there are not a lot of high-priority threads
		 * which will overflow the counter once again between the previous call and this one.
		 *
		 * NOTE: It is possible to have collisions after such overflow. */
		result.uuid_ = atomic_add_and_fetch_uint64(&global_session_uuid.uuid_, 1);
	}
	return result;
}

bool LIB_session_uuid_is_generated(const SessionUUID *uuid) {
	return !LIB_session_uuid_is_equal(uuid, &global_session_uuid_none);
}

bool LIB_session_uuid_is_equal(const SessionUUID *lhs, const SessionUUID *rhs) {
	return lhs->uuid_ == rhs->uuid_;
}

uint64_t LIB_session_uuid_hash_uint64(const SessionUUID *uuid) {
	return uuid->uuid_;
}

uint LIB_session_uuid_ghash_hash(const void *uuid_v) {
	const SessionUUID *uuid = (const SessionUUID *)uuid_v;
	return uuid->uuid_ & 0xffffffff;
}

bool LIB_session_uuid_ghash_compare(const void *lhs_v, const void *rhs_v) {
	const SessionUUID *lhs = (const SessionUUID *)lhs_v;
	const SessionUUID *rhs = (const SessionUUID *)rhs_v;
	return !LIB_session_uuid_is_equal(lhs, rhs);
}
