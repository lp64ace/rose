#ifndef DNA_ACTION_TYPES_H
#define DNA_ACTION_TYPES_H

#include "DNA_anim_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------- */
/** \name Action Group
 * \{ */

typedef struct ActionGroup {
	struct ActionGroup *prev, *next;

	char name[64];

	/**
	 * Span of channels in this group for layered actions.
	 *
	 * This specifies that span as a range of items in a Channelbag's fcurve
	 * array.
	 *
	 * Note that empty groups (`fcurve_range_length == 0`) are allowed, and they
	 * still have a position in the fcurves array, as specified by
	 * `fcurve_range_start`. You can imagine these cases as a zero-width range
	 * that sits at the border between the element at `fcurve_range_start` and the
	 * element just before it.
	 */
	int fcurve_range_start;
	int fcurve_range_length;

	/**
	 * For layered actions: the Channelbag this group belongs to.
	 *
	 * This is needed in the keyframe drawing code, etc., to give direct access to
	 * the fcurves in this group.
	 */
	struct ActionChannelBag *channelbag;
} ActionGroup;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Actions
 * \{ */

typedef struct Action {
	ID id;

	struct ActionLayer **layers;
	int totlayer;
	/**
	 * The index within the #layers array of the active action layer.
	 */
	int actlayer;

	struct ActionSlot **slots;
	int totslot;
	/**
	 * The last used slot handle, this is used to give a unique identifier to each slot.
	 * \note See #ActionSlot::handle for more information!
	 */
	int uidslot;

	struct ActionStripKeyframeData **stripkeyframedata;
	int totstripkeyframedata;

	int flag;

	ListBase markers;

	/**
	 * Start and end of the manually set intended playback frame range.
	 * \note This doesn't directly affect animation evaluation in any way.
	 */
	float frame_start;
	float frame_end;
} Action;

enum {
	/** The action has a manually set intended playback frame range. */
	ACT_FRAME_RANGE = (1 << 0),
	/**
	 * The action is cyclic (i.e. after the last frame it continues at the first one).
	 * requires ACT_FRAME_RANGE.
	 */
	ACT_CYCLIC = (1 << 1),
};

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Layer
 * \{ */

typedef struct ActionLayer {
	char name[64];

	float influence;

	int flag;
	int mix;

	struct ActionStrip **strips;
	int totstrip;
} ActionLayer;

#define ACTION_LAYER_DEFAULT_NAME "Layer"

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Slots
 * \{ */

typedef struct ActionSlotRuntime ActionSlotRuntime;

typedef struct ActionSlot {
	/**
	 * The string identifier of this Slot within the Action.
	 *
	 * The first two characters are the two-letter code corresponding to `idtype`
	 * below (e.g. 'OB', 'ME', 'LA'), and the remaining characters store slot's
	 * display name. Since the combination of the `idtype` and display name are
	 * always unique within an action, this string identifier is as well.
	 *
	 * Typically this matches the ID name this slot was created for, including the
	 * two letters indicating the ID type.
	 */
	char identifier[64];

	/**
	 * Type of ID-block that this slot is intended to be used with.
	 * 
	 * If 0, will be set to whatever ID-block type is used first.
	 */
	short idtype;

	/**
	 * Numeric identifier of this Slot within the action.
	 * 
	 * This number allows reorganization of the #Action::slots array without
	 * invalidating references. Also these remain valid when copy-on-evaluate
	 * copies are made.
	 * 
	 * Unlike `identifier` above, this cannobe be set by the user and never chanes 
	 * after initial assignment, and this serves as a "forever" identifier of the 
	 * slot.
	 * 
	 * Only valid within the Action that owns this slot.
	 */
	int handle;

	ActionSlotRuntime *runtime;
} ActionSlot;

#define ACTION_SLOT_DEFAULT_NAME "Slot"

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Strip
 * \{ */

typedef struct ActionStrip {
	int strip_type;
	/**
	 * The index of the "strip data" item that this strip uses, in the array of
	 * strip data that corresponds to `strip_type`.
	 *
	 * Note that -1 indicates "no data".  This is an invalid state outside of
	 * specific internal APIs, but it's the default value and therefore helps us
	 * catch when strips aren't fully initialized before making their way outside
	 * of those APIs.
	 */
	int index_data;

	float frame_start;
	float frame_end;
} ActionStrip;

typedef struct ActionStripKeyframeData {
	struct ActionChannelBag **channelbags;
	int totchannelbag;
} ActionStripKeyframeData;

typedef struct ActionChannelBag {
	int handle;

	/**
	 * Channel groups. These index into the `fcurve_array` below to specify group
	 * membership of the fcurves.
	 *
	 * Note that although the fcurves also have pointers back to the groups they
	 * belong to, those pointers are not the source of truth. The source of truth
	 * for membership is the information in the channel groups here.
	 *
	 * Invariants:
	 * 1. The groups are sorted by their `fcurve_range_start` field. In other
	 *    words, they are in the same order as their starting positions in the
	 *    fcurve array.
	 * 2. The grouped fcurves are tightly packed, starting at the first fcurve and
	 *    having no gaps of ungrouped fcurves between them. Ungrouped fcurves come
	 *    at the end, after all of the grouped fcurves.
	 */
	struct ActionGroup **groups;
	int totgroup;

	struct FCurve **fcurves;
	int totcurve;

	int flag;
} ActionChannelBag;

enum {
	ACTION_STRIP_KEYFRAME = 0,
};

/** \} */

/* #pchannel->rotmode, #object->rotmode */
enum {
	/* quaternion rotations (default, and for older Rose versions) */
	ROT_MODE_QUAT = 0,

	/* euler rotations - keep in sync with enum in LIB_math.h */
	/** Rose 'default' (classic) - must be as 1 to sync with LIB_math_rotation.h defines */
	ROT_MODE_EUL = 1,
	ROT_MODE_XYZ = 1,
	ROT_MODE_XZY = 2,
	ROT_MODE_YXZ = 3,
	ROT_MODE_YZX = 4,
	ROT_MODE_ZXY = 5,
	ROT_MODE_ZYX = 6,
	/**
	 * NOTE: space is reserved here for 18 other possible
	 * euler rotation orders not implemented.
	 */
	/* axis angle rotations */
	ROT_MODE_AXISANGLE = -1,

	ROT_MODE_MIN = ROT_MODE_AXISANGLE, /* sentinel for negative value. */
	ROT_MODE_MAX = ROT_MODE_ZYX,
};

/* -------------------------------------------------------------------- */
/** \name Action Pose
 * \{ */

typedef struct PoseChannel_Runtime {
	SessionUUID uuid;

	/** For display, pose_mat with bone length applied. */
	float disp_mat[4][4];
	/** For display, pose_mat with bone length applied and translated to tail. */
	float disp_tail_mat[4][4];

	struct PoseChannel *orig_pchannel;
} PoseChannel_Runtime;

typedef struct PoseChannel {
	struct PoseChannel *prev, *next;

	char name[64];

	/** Set on read or rebuild pose */
	struct Bone *bone;
	/** Set on read or rebuild pose */
	struct PoseChannel *parent;
	/** Set on read or rebuild pose */
	struct PoseChannel *child;

	/** Transforms - written in by actions or transform. */
	float loc[3];
	float scale[3];
	float euler[3];
	float quat[4];
	float rotAxis[3], rotAngle;
	
	int rotmode;
	int flag;

	/**
	 * Matrix result of location/rotation/scale components, and evaluation of
	 * animation data and constraints.
	 *
	 * This is the dynamic component of `pose_mat` (without #Bone.arm_mat).
	 */
	float chan_mat[4][4];
	/**
	 * Channel matrix in the armature object space, i.e. `pose_mat = bone->arm_mat * chan_mat`.
	 */
	float pose_mat[4][4];

	PoseChannel_Runtime runtime;
} PoseChannel;

enum {
	POSE_LOC = (1 << 0),
	POSE_ROT = (1 << 1),
	POSE_SCALE = (1 << 2),

	POSE_IKTREE = (1 << 8),
	POSE_DONE = (1 << 9),
};

typedef struct Pose {
	struct ListBase channelbase;
	struct GHash *channelhash;

	int flag;

	PoseChannel **channels;
} Pose;

enum {
	POSE_RECALC = (1 << 0),
	POSE_WAS_REBUILT = (1 << 1),
};

/** \} */

#ifdef __cplusplus
}
#endif

#endif // DNA_ACTION_TYPES_H
