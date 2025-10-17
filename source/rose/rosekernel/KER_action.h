#ifndef KER_ACTION_H
#define KER_ACTION_H

#include "DNA_action_types.h"

#include "KER_lib_query.h"

#include <stdbool.h>

struct Action;
struct ActionSlot;
struct Main;
struct Pose;
struct PoseChannel;

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*fnActionSlotCallback)(struct ID *animated, const struct Action *action, int handle, void *userdata);

bool KER_id_foreach_action_slot_use(struct ID *animated, fnActionSlotCallback callback, void *userdata);

/* -------------------------------------------------------------------- */
/** \name Action Layer
 * \{ */

/** Allocates and appends a new layer for the specified #Action. */
struct ActionLayer *KER_action_layer_add(struct Action *action, const char *name);

void KER_action_layer_free(struct ActionLayer *layer);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Strip
 * \{ */

/** Appends the specified #ActionStripKeyframeData and return the index of the data within the #Action. */
int KER_action_strip_keyframe_data_append(struct Action *action, struct ActionStripKeyframeData *strip_data);

/** Allocates and appends a new layer strip for the specified #ActionLayer. */
struct ActionStrip *KER_action_layer_strip_add(struct Action *action, struct ActionLayer *layer, int type);

void KER_action_strip_keyframe_data_free(struct ActionStripKeyframeData *strip_data);
void KER_action_strip_free(struct ActionStrip *strip);

struct ActionChannelBag *KER_action_strip_keyframe_data_channelbag_add_ex(struct ActionStripKeyframeData *strip_data, int handle);
struct ActionChannelBag *KER_action_strip_keyframe_data_channelbag_add(struct ActionStripKeyframeData *strip_data, struct ActionSlot *slot);
struct ActionChannelBag *KER_action_strip_keyframe_data_channelbag_for_slot_ex(struct ActionStripKeyframeData *strip_data, int handle);
struct ActionChannelBag *KER_action_strip_keyframe_data_channelbag_for_slot(struct ActionStripKeyframeData *strip_data, struct ActionSlot *slot);
struct ActionChannelBag *KER_action_strip_keyframe_data_ensure_channelbag_for_slot_ex(struct ActionStripKeyframeData *strip_data, int handle);
struct ActionChannelBag *KER_action_strip_keyframe_data_ensure_channelbag_for_slot(struct ActionStripKeyframeData *strip_data, struct ActionSlot *slot);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action
 * \{ */

void KER_action_keystrip_ensure(struct Action *action);
bool KER_action_assign(struct Action *action, struct ID *id);

bool KER_action_has_slot(const struct Action *action, const struct ActionSlot *slot);

struct ActionSlot *KER_action_slot_find_by_identifier(struct Action *action, const char *identifier);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Slot
 * \{ */

const struct ActionSlot *KER_action_slot_for_handle_const(const struct Action *action, int handle);
struct ActionSlot *KER_action_slot_for_handle(struct Action *action, int handle);

/** Allocates and append a new action slot for the specified #Action. */
struct ActionSlot *KER_action_slot_for_add_idtype(struct Action *action, short idtype);

void KER_action_slot_identifier_define(struct Action *action, struct ActionSlot *slot, const char *name);
void KER_action_slot_setup_for_id(struct ActionSlot *slot, const struct ID *id);
void KER_action_slot_free(struct ActionSlot *slot);

bool KER_action_slot_suitable_for_id(struct ActionSlot *slot, struct ID *id);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Action Runtime
 * \{ */

void KER_action_slot_foreach_id(ActionSlot *slot, struct LibraryForeachIDData *data);

void KER_action_slot_runtime_users_add(const struct ActionSlot *slot, struct ID *id);
void KER_action_slot_runtime_users_remove(const struct ActionSlot *slot, struct ID *id);
void KER_action_slot_runtime_users_invalidate(struct ActionSlot *slot, struct Main *main);
void KER_action_slot_runtime_free(struct ActionSlot *slot);

/** \} */

/* -------------------------------------------------------------------- */
/** \name PoseChannel Action
 * \{ */

/**
 * Looks to see if the channel with the given name already exists
 * in this pose - if not a new one is allocated and initialized.
 *
 * \note Use with care, not on Armature poses but for temporal ones.
 * \note (currently used for action constraints and in rebuild_pose).
 */
struct PoseChannel *KER_pose_channel_ensure(struct Pose *pose, const char *name);
struct PoseChannel *KER_pose_channel_find(struct Pose *pose, const char *name);

/**
 * Apply a 4x4 matrix to the pose bone,
 * similar to #KER_object_apply_mat4().
 */
void KER_pose_channel_apply_mat4(struct PoseChannel *pchannel, const float mat[4][4], bool use_compat);
void KER_pose_channel_mat3_to_rot(struct PoseChannel *pchannel, const float mat[3][3], bool use_compat);

void KER_pose_channel_free_ex(struct PoseChannel *pchan, bool do_id_user);

/** \} */

/* -------------------------------------------------------------------- */
/** \name Pose Action
 * \{ */

/**
 * Removes the hash for quick lookup of channels, must be done when adding/removing channels.
 */
void KER_pose_channels_hash_ensure(struct Pose *pose);
void KER_pose_channels_hash_free(struct Pose *pose);

/**
 * Return a pointer to the pose channel of the given name
 * from this pose.
 */
struct PoseChannel *KER_pose_channel_find_name(const struct Pose *pose, const char *name);

void KER_pose_free_data_ex(struct Pose *pose, const bool do_id_user);
void KER_pose_free_data(struct Pose *pose);
void KER_pose_free_ex(struct Pose *pose, const bool do_id_user);
void KER_pose_free(struct Pose *pose);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_ACTION_H