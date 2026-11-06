#ifndef NLA_PRIVATE_HH
#define NLA_PRIVATE_HH

#include "LIB_bitmap.h"
#include "LIB_ghash.h"
#include "LIB_listbase.h"
#include "LIB_map.hh"
#include "LIB_hash.hh"

struct NlaEvalChannel;
struct NlaEvalChannelKey;
struct NlaEvalChannelSnapshot;
struct NlaEvalData;
struct NlaEvalSnapshot;
struct NlaEvalStrip;
struct NlaStrip;

typedef struct NlaEvalStrip {
	struct NlaEvalStrip *prev, *next;

	NlaTrack *track;
	NlaStrip *strip;

	int index;
	int mode;

	float time;
} NlaEvalStrip;

/** #NlaEvalStrip->mode */
enum {
	/* standard evaluation */
	NES_TIME_BEFORE = -1,
	NES_TIME_WITHIN,
	NES_TIME_AFTER,

	/* transition-strip evaluations */
	NES_TIME_TRANSITION_START,
	NES_TIME_TRANSITION_END,
};

typedef struct NlaEvalChannelKey {
	struct PointerRNA ptr;
	struct PropertyRNA *property;

	friend bool operator==(const NlaEvalChannelKey &a, const NlaEvalChannelKey &b) {
		return a.ptr.data == b.ptr.data && a.property == b.property;
	}

	uint64_t hash() const {
		return rose::get_default_hash(this->ptr.data, this->property);
	}
} NlaEvalChannelKey;

/** Bitmask of array indices touched by actions. */
typedef struct NlaValidMask {
	BitMap *ptr;
	BitMap buffer[sizeof(uint64_t) / sizeof(BitMap)];
} NlaValidMask;

/** Set of property values for blending. */
typedef struct NlaEvalChannelSnapshot {
	struct NlaEvalChannel *channel;

	NlaValidMask blend_domain;
	NlaValidMask remap_domain;

	int length;
	
	unsigned int is_base : 1;

	float values[];
} NlaEvalChannelSnapshot;

/**
 * Temp channel for accumulating data from NLA for a single property.
 * Handles array properties as a unit to allow intelligent blending.
 */
typedef struct NlaEvalChannel {
	struct NlaEvalChannel *prev, *next;
	struct NlaEvalData *owner;

	/** Original RNA path string and property key */
	const char *rna_path;
	NlaEvalChannelKey key;

	int index;
	int mix;

	unsigned int is_array : 1;

	/* Associated with the RNA property's value(s), marks which elements are affected by NLA. */
	NlaValidMask domain;

	/* Base set of values. */
	NlaEvalChannelSnapshot base_snapshot;
} NlaEvalChannel;

/** #NlaEvalChannelSnapshot->mix */
enum {
	NEC_MIX_ADD,
	NEC_MIX_MULTIPLY,
	NEC_MIX_QUATERNION,
	NEC_MIX_AXIS_ANGLE,
};

/** Set of values for all channels. */
typedef struct NlaEvalSnapshot {
	/* Snapshot this one defaults to. */
	struct NlaEvalSnapshot *base;

	int size;

	NlaEvalChannelSnapshot **channels;
} NlaEvalSnapshot;

/** Set of all channels covered by NLA. */
typedef struct NlaEvalData {
	ListBase channels;
	int totchannels;

	/* Mapping of paths and NlaEvalChannelKeys to channels. */
	GHash *path_hash;
	rose::Map<NlaEvalChannelKey, NlaEvalChannel *> *key_hash;

	NlaEvalSnapshot base_snapshot;

	/* Evaluation result snapshot. */
	NlaEvalSnapshot eval_snapshot;
} NlaEvalData;

/**
 * Convert non clipped mapping for strip-time <-> global time:
 * `mode = eNlaTime_ConvertModes[] -> NLATIME_CONVERT_*`
 *
 * Only secure for 'internal' (i.e. within AnimSys evaluation) operations,
 * but should not be directly relied on for stuff which interacts with editors.
 */
float nla_strip_get_frame(struct NlaStrip *strip, float cframe, short mode);

typedef enum eNlaStripEvaluateMode {
	/** Blend upper strip with lower stack. */
	STRIP_EVAL_BLEND,
	/** Given upper strip and blended snapshot, solve for lower stack. */
	STRIP_EVAL_BLEND_GET_INVERTED_LOWER_SNAPSHOT,
	/**
	 * Store strip fcurve values in snapshot, properly marking blend_domain values.
	 *
	 * Currently only used for transitions to distinguish fcurve sampled values from default or lower
	 * stack values.
	 */
	STRIP_EVAL_NOBLEND,
} eNlaStripEvaluateMode;

void nla_strip_evaluate(const eNlaStripEvaluateMode evaluation_mode, struct PointerRNA *ptr, struct NlaEvalData *channels, struct ListBase *modifiers, struct NlaEvalStrip *nes, struct NlaEvalSnapshot *snapshot, float ctime, bool flush_to_original);

#endif // !NLA_PRIVATE_HH
