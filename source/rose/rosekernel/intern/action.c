#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_fcurve.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"
#include "KER_main.h"

#include "LIB_ghash.h"
#include "LIB_hash_mm2a.h"
#include "LIB_listbase.h"
#include "LIB_math_matrix.h"
#include "LIB_math_rotation.h"
#include "LIB_math_vector.h"
#include "LIB_hash.h"
#include "LIB_string.h"
#include "LIB_utildefines.h"

bool KER_id_foreach_action_slot_use(ID *animated, fnActionSlotCallback callback, void *userdata) {
	AnimData *adt = KER_animdata_from_id(animated);

	if (adt) {
		if (adt->action) {
			if (!callback(animated, adt->action, adt->handle, userdata)) {
				return false;
			}
		}
	}

	return true;
}

/* -------------------------------------------------------------------- */
/** \name Action Layer
 * \{ */

ROSE_STATIC ActionLayer *action_layer_alloc() {
	ActionLayer *layer = MEM_callocN(sizeof(ActionLayer), "ActionLayer");
	layer->influence = 1.0f;
	return layer;
}

ActionLayer *KER_action_layer_add(Action *action, const char *name) {
	ActionLayer *nlayer = action_layer_alloc();

	if (name) {
		LIB_strcpy(nlayer->name, ARRAY_SIZE(nlayer->name), name);
	}
	else {
		LIB_strcpy(nlayer->name, ARRAY_SIZE(nlayer->name), ACTION_LAYER_DEFAULT_NAME);
	}

	// Increment first, so we can use it directly.
	action->totlayer++;
	action->layers = (ActionLayer **)MEM_reallocN(action->layers, sizeof(ActionLayer *) * (action->totlayer));
	action->layers[action->totlayer - 1] = nlayer;
	action->actlayer = action->totlayer - 1;

	return nlayer;
}

void KER_action_layer_free(ActionLayer *layer) {
	for (size_t index = 0; index < (size_t)layer->totstrip; index++) {
		KER_action_strip_free(layer->strips[index]);
	}
	MEM_SAFE_FREE(layer->strips);

	MEM_freeN(layer);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action ChannelBag
 * \{ */

void KER_action_group_free(ActionGroup *group) {
	MEM_freeN(group);
}

void KER_action_channelbag_free(ActionChannelBag *channelbag) {
	for (size_t index = 0; index < (size_t)channelbag->totcurve; index++) {
		KER_fcurve_free(channelbag->fcurves[index]);
	}
	MEM_SAFE_FREE(channelbag->fcurves);

	for (size_t index = 0; index < (size_t)channelbag->totgroup; index++) {
		KER_action_group_free(channelbag->groups[index]);
	}
	MEM_SAFE_FREE(channelbag->groups);
	MEM_freeN(channelbag);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Strip
 * \{ */

const ActionStripKeyframeData *KER_action_strip_data_const(const Action *action, const ActionStrip *strip) {
	ROSE_assert(0 <= strip->index_data && strip->index_data < action->totstripkeyframedata);

	return action->stripkeyframedata[strip->index_data];
}

ActionStripKeyframeData *KER_action_strip_data(Action *action, ActionStrip *strip) {
	ROSE_assert(0 <= strip->index_data && strip->index_data < action->totstripkeyframedata);

	return action->stripkeyframedata[strip->index_data];
}

ROSE_STATIC ActionStrip *action_strip_alloc(Action *action, int type) {
	ActionStrip *strip = MEM_callocN(sizeof(ActionStrip), "ActionStrip");

	strip->index_data = -1;
	strip->frame_start = -INFINITY;
	strip->frame_end = INFINITY;
	strip->strip_type = type;

	switch (type) {
		case ACTION_STRIP_KEYFRAME: {
			ActionStripKeyframeData *strip_data = MEM_callocN(sizeof(ActionStripKeyframeData), "ActionStripKeyframeData");
			strip->index_data = KER_action_strip_keyframe_data_append(action, strip_data);
		} break;
	}

	return strip;
}

int KER_action_strip_keyframe_data_append(Action *action, ActionStripKeyframeData *strip_data) {
	action->totstripkeyframedata++;
	action->stripkeyframedata = (ActionStripKeyframeData **)MEM_reallocN(action->stripkeyframedata, sizeof(ActionStripKeyframeData *) * action->totstripkeyframedata);
	action->stripkeyframedata[action->totstripkeyframedata - 1] = strip_data;
	return action->totstripkeyframedata - 1;
}

ActionStrip *KER_action_layer_strip_add(Action *action, ActionLayer *layer, int type) {
	ActionStrip *nstrip = action_strip_alloc(action, type);

	// Increment first, so we can use it directly.
	layer->totstrip++;
	layer->strips = (ActionStrip **)MEM_reallocN(layer->strips, sizeof(ActionStrip *) * (layer->totstrip));
	layer->strips[layer->totstrip - 1] = nstrip;

	return nstrip;
}

void KER_action_strip_keyframe_data_free(ActionStripKeyframeData *strip_data) {
	for (size_t index = 0; index < (size_t)strip_data->totchannelbag; index++) {
		if (strip_data->channelbags[index]) {
			KER_action_channelbag_free(strip_data->channelbags[index]);
		}
	}
	MEM_SAFE_FREE(strip_data->channelbags);
	MEM_freeN(strip_data);
}

void KER_action_strip_free(ActionStrip *strip) {
	MEM_freeN(strip);
}

ActionChannelBag *KER_action_strip_keyframe_data_channelbag_add_ex(ActionStripKeyframeData *strip_data, int handle) {
	ROSE_assert(KER_action_strip_keyframe_data_channelbag_for_slot_ex(strip_data, handle) == NULL);
	ROSE_assert(handle != 0);

	ActionChannelBag *nchannelbag = MEM_callocN(sizeof(ActionChannelBag), "ActionChannelBag");
	nchannelbag->handle = handle;

	strip_data->totchannelbag++;
	strip_data->channelbags = (ActionChannelBag **)MEM_reallocN(strip_data->channelbags, sizeof(ActionChannelBag *) * strip_data->totchannelbag);
	strip_data->channelbags[strip_data->totchannelbag - 1] = nchannelbag;

	return nchannelbag;
}

ActionChannelBag *KER_action_strip_keyframe_data_channelbag_add(ActionStripKeyframeData *strip_data, ActionSlot *slot) {
	return KER_action_strip_keyframe_data_channelbag_add_ex(strip_data, slot->handle);
}

ActionChannelBag *KER_action_strip_keyframe_data_channelbag_for_slot_ex(ActionStripKeyframeData *strip_data, int handle) {
	for (size_t index = 0; index < (size_t)strip_data->totchannelbag; index++) {
		ActionChannelBag *channelbag = strip_data->channelbags[index];
		if (channelbag->handle == handle) {
			return channelbag;
		}
	}
	return NULL;
}
ActionChannelBag *KER_action_strip_keyframe_data_channelbag_for_slot(ActionStripKeyframeData *strip_data, ActionSlot *slot) {
	return KER_action_strip_keyframe_data_channelbag_for_slot_ex(strip_data, slot->handle);
}

ActionChannelBag *KER_action_strip_keyframe_data_ensure_channelbag_for_slot_ex(ActionStripKeyframeData *strip_data, int handle) {
	ActionChannelBag *channelbag = KER_action_strip_keyframe_data_channelbag_for_slot_ex(strip_data, handle);
	if (channelbag != NULL) {
		return channelbag;
	}
	return KER_action_strip_keyframe_data_channelbag_add_ex(strip_data, handle);
}
ActionChannelBag *KER_action_strip_keyframe_data_ensure_channelbag_for_slot(ActionStripKeyframeData *strip_data, ActionSlot *slot) {
	return KER_action_strip_keyframe_data_ensure_channelbag_for_slot_ex(strip_data, slot->handle);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action
 * \{ */

void KER_action_keystrip_ensure(Action *action) {
	ActionLayer *layer;
	if (action->totlayer == 0) {
		layer = KER_action_layer_add(action, NULL);
	}
	else {
		layer = action->layers[0];
	}

	if (layer->totstrip == 0) {
		KER_action_layer_strip_add(action, layer, ACTION_STRIP_KEYFRAME);
	}
}

typedef struct VisitActionUseData {
	Action *action;
	int handle;
} VisitActionUseData;

ROSE_STATIC bool visit_action_use(struct ID *animated, const struct Action *used_action, int used_handle, void *userdata) {
	VisitActionUseData *data = (VisitActionUseData *)userdata;

	if (used_action != data->action) {
		return true;
	}
	if (used_handle != data->handle) {
		return true;
	}

	return false; // We found a match, the iteration should exit and return false!
}

bool is_id_using_action_slot(ID *id, Action *action, int handle) {
	VisitActionUseData data = {
		.action = action,
		.handle = handle,
	};

	return KER_id_foreach_action_slot_use(id, visit_action_use, &data) == false;
}

ROSE_STATIC bool generic_assign_action_slot(ActionSlot *slot, ID *id, Action **action_ptr, int *r_handle) {
	if (!(*action_ptr)) {
		/* No action assigned yet, so no way to assign a slot. */
		return false;
	}

	Action *action = *action_ptr;

	if (slot) {
		if (!KER_action_has_slot(action, slot)) {
			return false;
		}

		if (!KER_action_slot_suitable_for_id(slot, id)) {
			return false;
		}
	}

	ActionSlot *prevslot = KER_action_slot_for_handle(action, *r_handle);

	*r_handle = 0;
	if (prevslot) {
		if (!is_id_using_action_slot(id, action, prevslot->handle)) {
			KER_action_slot_runtime_users_remove(prevslot, id);
		}
	}

	if (!slot) {
		return true;
	}

	KER_action_slot_setup_for_id(slot, id);
	*r_handle = slot->handle;
	KER_action_slot_runtime_users_add(slot, id);

	return true;
}

ROSE_STATIC ActionSlot *generic_slot_for_autoassign(Action *action, ID *id) {
	/* Search for the ID name (which includes the ID type). */
	{
		ActionSlot *slot = KER_action_slot_find_by_identifier(action, id->name);
		if (slot && KER_action_slot_suitable_for_id(slot, id)) {
			return slot;
		}
	}

	/* If there is only one slot, and it is not specific to any ID type, use that.
	 *
	 * This should only trigger in some special cases, like legacy Actions that were converted to
	 * slotted Actions by the versioning code, where the legacy Action was never assigned to anything
	 * (and thus had idroot = 0).
	 *
	 * This might seem overly specific, and for convenience of automatically auto-assigning a slot,
	 * it might be tempting to remove the "slot->has_idtype()" check. However, that would make the
	 * following workflow significantly more cumbersome:
	 *
	 * - Animate `Cube`. This creates `CubeAction` with a single slot `OBCube`.
	 * - Assign `CubeAction` to `Suzanne`, with the intent of animating both `Cube` and `Suzanne`
	 *   with the same Action.
	 * - This should **not** auto-assign the `OBCube` slot to `Suzanne`, as that will overwrite any
	 *   property of `Suzanne` with the animated values for the `OBCube` slot.
	 *
	 * Recovering from this will be hard, as an undo will revert both the overwriting of properties
	 * and the assignment of the Action. */
	if (action->totslot == 1) {
		ActionSlot *slot = action->slots[0];
		if (slot->idtype == 0) {
			return slot;
		}
	}

	return NULL;
}

ROSE_STATIC bool generic_assign_action(ID *id, Action *action_to_assign, Action **action_ptr, int *r_handle) {
	/* Un-assign any previously-assigned Action first. */
	if (*action_ptr) {
		if (*r_handle) {
			generic_assign_action_slot(NULL, id, action_ptr, r_handle);
		}

		id_us_rem(&(*action_ptr)->id);
		*action_ptr = NULL;
	}

	if (!action_to_assign) {
		/* Un-assigning was the point, so the work is done. */
		return true;
	}

	*action_ptr = action_to_assign;
	id_us_add(&(*action_ptr)->id);

	ActionSlot *slot = generic_slot_for_autoassign(*action_ptr, id);
	return generic_assign_action_slot(slot, id, action_ptr, r_handle);
}

bool KER_action_assign(Action *action, ID *id) {
	AnimData *adt = KER_animdata_ensure_id(id);
	if (!adt) {
		return false;
	}
	return generic_assign_action(id, action, &adt->action, &adt->handle);
}

bool KER_action_slot_assign(ActionSlot *slot, ID *id) {
	AnimData *adt = KER_animdata_ensure_id(id);
	if (!adt) {
		return false;
	}
	return generic_assign_action_slot(slot, id, &adt->action, &adt->handle);
}

bool KER_action_has_slot(const struct Action *action, const struct ActionSlot *slot) {
	for (size_t index = 0; index < (size_t)action->totslot; index++) {
		if (action->slots[index] == slot) {
			return true;
		}
	}
	return false;
}

ActionSlot *KER_action_slot_find_by_identifier(Action *action, const char *identifier) {
	for (size_t index = 0; index < (size_t)action->totslot; index++) {
		if (action->slots[index] && STREQ(action->slots[index]->identifier, identifier)) {
			return action->slots[index];
		}
	}
	return NULL;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Slot
 * \{ */

ROSE_STATIC ActionSlot *action_slot_alloc(Action *action) {
	ActionSlot *slot = MEM_callocN(sizeof(ActionSlot), "ActionSlot");

	action->uidslot++;
	ROSE_assert_msg(action->uidslot > 0, "Action slot handle overflow");

	slot->handle = action->uidslot;

	KER_action_slot_runtime_init(slot);
	
	return slot;
}

ActionSlot *KER_action_slot_add(Action *action) {
	ActionSlot *slot = action_slot_alloc(action);

	/* Assign the default name and the 'untyped' identifier prefix. */
	*(short *)slot->identifier = MAKE_ID2('X', 'X');
	LIB_strcpy(slot->identifier + 2, ARRAY_SIZE(slot->identifier) - 2, ACTION_SLOT_DEFAULT_NAME);

	action->totslot++;
	action->slots = (ActionSlot **)MEM_reallocN(action->slots, sizeof(ActionSlot *) * action->totslot);
	action->slots[action->totslot - 1] = slot;

	return slot;
}

ActionSlot *KER_action_slot_add_for_idtype(struct Action *action, short idtype) {
	ActionSlot *nslot = KER_action_slot_add(action);

	nslot->idtype = idtype;
	*(short *)nslot->identifier = idtype;
	LIB_strcpy(nslot->identifier + 2, ARRAY_SIZE(nslot->identifier) - 2, ACTION_SLOT_DEFAULT_NAME);

	return nslot;
}

void KER_action_slot_identifier_define(Action *action, ActionSlot *slot, const char *name) {
	if (!name) {
		name = ACTION_SLOT_DEFAULT_NAME;
	}
	LIB_strcpy(slot->identifier + 2, ARRAY_SIZE(slot->identifier) - 2, name);

	UNUSED_VARS(action);  // We may want to ensure uniqueness in the future.
}

void KER_action_slot_setup_for_id(ActionSlot *slot, const ID *id) {
	if (slot->idtype != 0) {
		ROSE_assert(slot->idtype == GS(id->name));
		return;
	}

	slot->idtype = GS(id->name);
	*(short *)slot->identifier = slot->idtype;
}

const ActionSlot *KER_action_slot_for_handle_const(const Action *action, int handle) {
	if (handle == 0) {
		return NULL;
	}

	for (size_t index = 0; index < (size_t)action->totslot; index++) {
		if (action->slots[index] && action->slots[index]->handle == handle) {
			return action->slots[index];
		}
	}

	return NULL;
}

ActionSlot *KER_action_slot_for_handle(Action *action, int handle) {
	if (handle == 0) {
		return NULL;
	}

	for (size_t index = 0; index < (size_t)action->totslot; index++) {
		if (action->slots[index] && action->slots[index]->handle == handle) {
			return action->slots[index];
		}
	}

	return NULL;
}

bool KER_action_slot_suitable_for_id(ActionSlot *slot, ID *id) {
	if (slot->idtype == 0) {
		/* Without specific ID type set, this Slot can animate any ID. */
		return true;
	}

	/* Check that the ID type is compatible with this slot. */
	const short idtype = GS(id->name);
	return slot->idtype == idtype;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name PoseChannel Action
 * \{ */

void KER_pose_tag_recalc(Pose *pose) {
	pose->flag |= POSE_RECALC;
}

PoseChannel *KER_pose_channel_ensure(Pose *pose, const char *name) {
	PoseChannel *chan;

	if (pose == NULL) {
		return NULL;
	}

	/* See if this channel exists */
	chan = KER_pose_channel_find_name(pose, name);
	if (chan) {
		return chan;
	}

	/* If not, create it and add it */
	chan = MEM_callocN(sizeof(PoseChannel), "VerifyPoseChannel");

	LIB_strcpy(chan->name, ARRAY_SIZE(chan->name), name);

	LIB_addtail(&pose->channelbase, chan);
	if (pose->channelhash) {
		LIB_ghash_insert(pose->channelhash, chan->name, chan);
	}

	return chan;
}

struct PoseChannel *KER_pose_channel_find(struct Pose *pose, const char *name) {
	if (ELEM(NULL, pose, name) || (name[0] == '\0')) {
		return NULL;
	}

	if (pose->channelhash) {
		return (PoseChannel *)LIB_ghash_lookup(pose->channelhash, (const void *)name);
	}

	return (PoseChannel *)LIB_findstr(&pose->channelbase, name, offsetof(PoseChannel, name));
}

void KER_pose_channel_mat3_to_rot(PoseChannel *pchannel, const float mat[3][3], bool use_compat) {
	switch (pchannel->rotmode) {
		case ROT_MODE_QUAT:
			mat3_normalized_to_quat(pchannel->quat, mat);
			break;
		case ROT_MODE_AXISANGLE:
			mat3_normalized_to_axis_angle(pchannel->rotAxis, &pchannel->rotAngle, mat);
			break;
		default: /* euler */
			if (use_compat) {
				mat3_normalized_to_compatible_eulO(pchannel->euler, pchannel->euler, pchannel->rotmode, mat);
			}
			else {
				mat3_normalized_to_eulO(pchannel->euler, pchannel->rotmode, mat);
			}
			break;
	}
}

void KER_pose_channel_apply_mat4(PoseChannel *pchannel, const float mat[4][4], bool use_compat) {
	float rot[3][3];
	mat4_to_loc_rot_size(pchannel->loc, rot, pchannel->scale, mat);
	KER_pose_channel_mat3_to_rot(pchannel, rot, use_compat);
}

void KER_pose_channel_free_ex(PoseChannel *pchan, bool do_id_user) {
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pose Action
 * \{ */

void KER_pose_channels_hash_ensure(Pose *pose) {
	if (!pose->channelhash) {
		pose->channelhash = LIB_ghash_str_new("Pose::channelhash");
		LISTBASE_FOREACH(PoseChannel *, pchan, &pose->channelbase) {
			LIB_ghash_insert(pose->channelhash, pchan->name, pchan);
		}
	}
}

void KER_pose_channels_hash_free(Pose *pose) {
	if (pose->channelhash) {
		LIB_ghash_free(pose->channelhash, NULL, NULL);
		pose->channelhash = NULL;
	}
}

PoseChannel *KER_pose_channel_find_name(const Pose *pose, const char *name) {
	if (ELEM(NULL, pose, name) || (name[0] == '\0')) {
		return NULL;
	}

	if (pose->channelhash) {
		return (PoseChannel *)LIB_ghash_lookup(pose->channelhash, (const void *)name);
	}

	return (PoseChannel *)LIB_findstr(&pose->channelbase, name, offsetof(PoseChannel, name));
}

void KER_pose_channels_free_ex(Pose *pose, const bool do_id_user) {
	if (!LIB_listbase_is_empty(&pose->channelbase)) {
		LISTBASE_FOREACH(PoseChannel *, pchannel, &pose->channelbase) {
			KER_pose_channel_free_ex(pchannel, do_id_user);
		}
		LIB_freelistN(&pose->channelbase);
	}

	KER_pose_channels_hash_free(pose);

	MEM_SAFE_FREE(pose->channels);
}

void KER_pose_channels_free(Pose *pose) {
	KER_pose_channels_free_ex(pose, true);
}

void KER_pose_free_data_ex(Pose *pose, const bool do_id_user) {
	KER_pose_channels_free_ex(pose, do_id_user);
}

void KER_pose_free_data(Pose *pose) {
	KER_pose_free_data_ex(pose, true);
}

void KER_pose_free_ex(Pose *pose, const bool do_id_user) {
	if (pose) {
		KER_pose_free_data_ex(pose, do_id_user);
		MEM_freeN(pose);
	}
}

void KER_pose_free(Pose *pose) {
	KER_pose_free_ex(pose, true);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Channel Bag
 * \{ */

ROSE_INLINE uint fcurve_lookup_hash(const void *key) {
	const FCurve *info = (const FCurve *)key;

	uint h0 = LIB_ghashutil_strhash(info->path);
	uint h1 = LIB_ghashutil_inthash(info->index);

	return h0 ^ h1;
}

ROSE_INLINE bool fcurve_lookup_cmp(const void *vleft, const void *vright) {
	const FCurve *left = (const FCurve *)vleft;
	const FCurve *right = (const FCurve *)vright;

	if (left->index == right->index && STREQ(left->path, right->path)) {
		return false;
	}
	return true;
}

ROSE_STATIC FCurve *create_fcurve_for_channel(const FCurveDescriptor *descriptor) {
	FCurve *fcurve = KER_fcurve_new();
	fcurve->path = LIB_strdupN(descriptor->path);
	fcurve->index = descriptor->index;
	
	if (descriptor->type >= 0) {
		// Note the type within the flag of the FCurve.
	}

	return fcurve;
}

ActionGroup *KER_action_channelbag_group_find(ActionChannelBag *channelbag, const char *name) {
	for (ActionGroup **group = channelbag->groups; group != channelbag->groups + channelbag->totgroup; group++) {
		if (*group != NULL && STREQ((*group)->name, name)) {
			return *group;
		}
	}
	return NULL;
}

int KER_action_channelbag_group_find_index(ActionChannelBag *channelbag, ActionGroup *group) {
	for (int index = 0; index < channelbag->totgroup; index++) {
		if (channelbag->groups[index] == group) {
			return index;
		}
	}
	return -1;
}

ActionGroup *KER_action_channelbag_group_create(ActionChannelBag *channelbag, const char *name) {
	ActionGroup *newgroup = MEM_callocN(sizeof(ActionGroup), "ActionGroup");

	int index = 0;
	const int length = channelbag->totgroup;
	if (length > 0) {
		ActionGroup *last = channelbag->groups[length - 1];
		index = last->fcurve_range_start + last->fcurve_range_length;
	}
	newgroup->fcurve_range_start = index;
	newgroup->channelbag = channelbag;
	
	LIB_strcpy(newgroup->name, ARRAY_SIZE(newgroup->name), name);

	channelbag->groups = MEM_reallocN(channelbag->groups, sizeof(ActionGroup *) * (length + 1));
	channelbag->groups[channelbag->totgroup++] = newgroup;

	return newgroup;
}

ActionGroup *KER_action_channelbag_group_ensure(ActionChannelBag *channelbag, const char *name) {
	ActionGroup *group = KER_action_channelbag_group_find(channelbag, name);
	if (group != NULL) {
		return group;
	}

	return KER_action_channelbag_group_create(channelbag, name);
}

ROSE_STATIC FCurve **fcurve_rotate(FCurve **first, FCurve **middle, FCurve **last) {
	if (first == middle) {
		return last;
	}
	if (middle == last) {
		return first;
	}

	FCurve **write = first;
	FCurve **next = first;

	for (FCurve **read = middle; read != last; ++write, ++read) {
		if (write == next) {
			next = read;
		}
		SWAP(FCurve *, write[0], read[0]);
	}

	fcurve_rotate(write, next, last);
	return write;
}

ROSE_STATIC void fcurve_shift_range(FCurve **fcurves, const int num, const int left, const int right, const int to) {
	ROSE_assert(left <= right);
	ROSE_assert(right <= num);
	ROSE_assert(to <= num + left - right);

	if (ELEM(left, right, to)) {
		return;
	}

	if (to < left) {
		FCurve **start = fcurves + to;
		FCurve **mid = fcurves + left;
		FCurve **end = fcurves + right;
		fcurve_rotate(start, mid, end);
	}
	else {
		FCurve **start = fcurves + left;
		FCurve **mid = fcurves + right;
		FCurve **end = fcurves + to;
		fcurve_rotate(start, mid, end);
	}
}

ROSE_STATIC void restore_channelbag_group_invariants(ActionChannelBag *channelbag) {
	{
		int start = 0;
		for (ActionGroup **group = channelbag->groups; group != channelbag->groups + channelbag->totgroup; group++) {
			group[0]->fcurve_range_start = start;
			start += group[0]->fcurve_range_length;
		}

		ROSE_assert(start <= channelbag->totcurve);
	}
	{
		for (FCurve **fcurve = channelbag->fcurves; fcurve != channelbag->fcurves + channelbag->totcurve; fcurve++) {
			fcurve[0]->group = NULL;
		}
		for (ActionGroup **group = channelbag->groups; group != channelbag->groups + channelbag->totgroup; group++) {
			for (size_t offset = 0; offset < group[0]->fcurve_range_length; offset++) {
				size_t index = offset + group[0]->fcurve_range_start;

				channelbag->fcurves[index]->group = group[0];
			}
		}
	}
}

void KER_action_channelbag_fcurve_create_many(Main *main, ActionChannelBag *channelbag, const FCurveDescriptor *descriptors, int length, FCurve **newcurves) {
	const int prevcount = channelbag->totcurve;
	
	GSet *unique = LIB_gset_new_ex(fcurve_lookup_hash, fcurve_lookup_cmp, "FCurveLookup", prevcount);
	for (FCurve **curve = channelbag->fcurves; curve != channelbag->fcurves + channelbag->totcurve; curve++) {
		LIB_gset_insert(unique, *curve);
	}

	channelbag->totcurve += length;
	channelbag->fcurves = MEM_reallocN(channelbag->fcurves, sizeof(FCurve *) * channelbag->totcurve);

	int newindex = prevcount;
	for (int i = 0; i < length; i++) {
		const FCurveDescriptor *descriptor = &descriptors[i];
		char *path = LIB_strdupN(descriptor->path);

		FCurve template = {
			.path = path,
			.index = descriptor->index,
		};

		if (descriptor->path == NULL || STREQ(descriptor->path, "") || LIB_gset_lookup(unique, &template)) {
			newcurves[i] = NULL;
			continue;
		}

		MEM_freeN(path);

		FCurve *fcurve = create_fcurve_for_channel(descriptor);
		newcurves[i] = fcurve;

		channelbag->fcurves[newindex] = fcurve;
		if (descriptor->group) {
			ActionGroup *group = KER_action_channelbag_group_ensure(channelbag, descriptor->group);
			const int groupindex = group->fcurve_range_start + group->fcurve_range_length;
			ROSE_assert(groupindex <= channelbag->totcurve);

			fcurve_shift_range(channelbag->fcurves, channelbag->totcurve, newindex, newindex + 1, groupindex);

			group->fcurve_range_length++;
			int index = KER_action_channelbag_group_find_index(channelbag, group);
			ROSE_assert(0 <= index && index < channelbag->totgroup);
			for (index = index + 1; index < channelbag->totgroup; index++) {
				channelbag->groups[index]->fcurve_range_start++;
			}
		}
		newindex++;
	}

	if (channelbag->totcurve != newindex) {
		// Some curves were not created, resize to final amount.
		channelbag->totcurve = newindex;
		channelbag->fcurves = MEM_reallocN(channelbag->fcurves, sizeof(FCurve *) * channelbag->totcurve);
	}

	LIB_gset_free(unique, NULL);

	restore_channelbag_group_invariants(channelbag);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Data-block Definition
 * \{ */

ROSE_STATIC void action_init_data(ID *id) {
	Action *action = (Action *)id;

	action->uidslot = 0x37627bf5;
}

ROSE_STATIC void action_free_data(ID *id) {
	Action *action = (Action *)id;
	
	for (size_t index = 0; index < (size_t)action->totstripkeyframedata; index++) {
		if (action->stripkeyframedata[index]) {
			KER_action_strip_keyframe_data_free(action->stripkeyframedata[index]);
		}
	}
	MEM_SAFE_FREE(action->stripkeyframedata);

	action->totstripkeyframedata = 0;

	for (size_t index = 0; index < (size_t)action->totlayer; index++) {
		if (action->layers[index]) {
			KER_action_layer_free(action->layers[index]);
		}
	}
	MEM_SAFE_FREE(action->layers);

	action->totlayer = 0;

	for (size_t index = 0; index < (size_t)action->totslot; index++) {
		if (action->slots[index]) {
			KER_action_slot_free(action->slots[index]);
		}
	}
	MEM_SAFE_FREE(action->slots);

	action->totslot = 0;
}

ROSE_STATIC void action_foreach_id(ID *id, struct LibraryForeachIDData *data) {
	Action *action = (Action *)id;
	for (size_t index = 0; index < (size_t)action->totslot; index++) {
		if (action->slots[index]) {
			KER_action_slot_foreach_id(action->slots[index], data);
		}
	}
}

IDTypeInfo IDType_ID_AC = {
	.idcode = ID_AC,

	.filter = FILTER_ID_AC,
	.depends = 0,
	.index = INDEX_ID_AC,
	.size = sizeof(Action),

	.name = "Action",
	.name_plural = "Actions",

	.flag = IDTYPE_FLAGS_NO_COPY | IDTYPE_FLAGS_NO_ANIMDATA,

	.init_data = action_init_data,
	.copy_data = NULL,
	.free_data = action_free_data,

	.foreach_id = action_foreach_id,

	.write = NULL,
	.read_data = NULL,
};

/** \} */
