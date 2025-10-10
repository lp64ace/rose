#include "MEM_guardedalloc.h"

#include "KER_action.h"
#include "KER_anim_data.h"
#include "KER_fcurve.h"
#include "KER_idtype.h"
#include "KER_lib_id.h"

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
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Strip
 * \{ */

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

ROSE_STATIC bool generatic_assign_action_slot(ActionSlot *slot, ID *id, Action **action_ptr, int *r_handle) {
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
			generatic_assign_action_slot(NULL, id, action_ptr, r_handle);
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
	return generatic_assign_action_slot(slot, id, action_ptr, r_handle);
}

bool KER_action_assign(Action *action, ID *id) {
	AnimData *adt = KER_animdata_ensure_id(id);
	if (!adt) {
		return false;
	}
	return generic_assign_action(id, action, &adt->action, &adt->handle);
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

ActionSlot *KER_action_slot_for_add_idtype(struct Action *action, short idtype) {
	ActionSlot *nslot = KER_action_slot_add(action);

	nslot->idtype = idtype;
	*(short *)nslot->identifier = idtype;
	LIB_strcpy(nslot->identifier + 2, ARRAY_SIZE(nslot->identifier) - 2, ACTION_SLOT_DEFAULT_NAME);

	return nslot;
}

void KER_action_slot_identifier_define(Action *action, ActionSlot *slot, const char *name) {
	LIB_strcpy(slot->identifier + 2, ARRAY_SIZE(slot->identifier) - 2, ACTION_SLOT_DEFAULT_NAME);

	UNUSED_VARS(action);  // We may want to ensure uniqueness in the future.
}

void KER_action_slot_setup_for_id(ActionSlot *slot, const ID *id) {
	if (slot->idtype != 0) {
		ROSE_assert(slot->idtype != GS(id->name));
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
/** \name Action Data-block Definition
 * \{ */

ROSE_STATIC void action_init_data(ID *id) {
	Action *action = (Action *)id;

	action->uidslot = (0x37627bf5 ^ POINTER_AS_INT(action)) >> 1;
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
