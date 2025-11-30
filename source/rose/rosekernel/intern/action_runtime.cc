#include "KER_action.hh"
#include "KER_anim_data.h"
#include "KER_main.h"

#include "LIB_set.hh"
#include "LIB_vector.hh"

typedef struct ActionSlotRuntime {
	/**
	 * Cache of pointers to the IDs that are animated by this slot.
	 *
	 * Note that this is a vector for simplicity, as the majority of the slots
	 * will have zero or one user. Semantically it's treated as a set: order
	 * doesn't matter, and it has no duplicate entries.
	 *
	 * \note This is NOT thread-safe.
	 */
	rose::Vector<ID *> users;
} ActionSlotRuntime;

ROSE_STATIC bool rebuild_slot_user_cache_for_id(ID *animated, const Action *action, int handle, void *unused) {
	const ActionSlot *slot = KER_action_slot_for_handle_const(action, handle);
	if (!slot) {
		return true;
	}
	/* Constant cast because the `foreach` produces const Actions, and I (Sybren)
	 * didn't want to make a non-const duplicate. */
	KER_action_slot_runtime_users_add(slot, animated);
	return true;
}

ROSE_STATIC void rebuild_slot_user_cache(Main *main) {
	LISTBASE_FOREACH(Action *, action, &main->actions) {
		for (ActionSlot *slot : rose::Span<ActionSlot *>(action->slots, static_cast<size_t>(action->totslot))) {
			ROSE_assert_msg(slot->runtime, "ActionSlot::runtime should always be allocated");
			slot->runtime->users.clear();
		}
	}

	/* Mark all Slots as clear. This is a bit of a lie, because the code below still has to run.
	 * However, this is a necessity to make the `slot.users_add(*id)` call work without triggering
	 * an infinite recursion.
	 *
	 * The alternative would be to go around the `slot.users_add()` function and access the
	 * runtime directly, but this is IMO a bit cleaner. */
	main->is_action_slot_to_id_map_dirty = false;

	rose::Set<ID *> visited;
	auto visit_id = [&visited](ID *id) -> bool {
		if (!visited.add(id)) {
			return false;
		}

		KER_id_foreach_action_slot_use(id, rebuild_slot_user_cache_for_id, NULL);

		return true;
	};

	ListBase *lb[INDEX_ID_MAX];
	int a = set_listbasepointers(main, lb);

	while (a--) {
		ID *id = (ID *)lb[a]->first;
		if (!id_can_have_animdata(id)) {
			continue;
		}

		LISTBASE_FOREACH(ID *, id, lb[a]) {
			ROSE_assert(id_can_have_animdata(id));

			if (!visit_id(id)) {
				continue;
			}
		}
	}
}

void KER_action_slot_foreach_id(ActionSlot *slot, struct LibraryForeachIDData *data) {
	ActionSlotRuntime *runtime = static_cast<ActionSlotRuntime *>(slot->runtime);

	/* When this function is called without the IDWALK_READONLY flag, calls to
	 * BKE_LIB_FOREACHID_PROCESS_... macros can change ID pointers. ID remapping is the main example
	 * of such use.
	 *
	 * Those ID pointer changes are not guaranteed to be valid, though. For example, the remapping
	 * can be used to replace one Mesh with another, but that neither means that the new Mesh is
	 * animated with the same Action, nor that the old Mesh is no longer animated by that Action. In
	 * other words, the best that can be done is to invalidate the cache.
	 *
	 * NOTE: early-returns by BKE_LIB_FOREACHID_PROCESS_... macros are forbidden in non-readonly
	 * cases (see #IDWALK_RET_STOP_ITER documentation). */

	constexpr int flag = IDWALK_CB_NEVER_SELF;

	if (runtime) {
		bool should_invalidate = false;

		for (ID *&user : runtime->users) {
			ID *const puser = user;
			KER_LIB_FOREACHID_PROCESS_ID(data, puser, flag);
			/* If slot_user changed, the cache should be invalidated. Not all pointer changes are
			 * semantically correct for our use. For example, when ID-remapping is used to replace
			 * MECube with MESuzanne. If MECube is animated by some slot before the remap, it will
			 * remain animated by that slot after the remap, even when all `object->data` pointers now
			 * reference MESuzanne instead. */
			should_invalidate |= (user != puser);
		}

		Main *main = KER_lib_query_foreachid_process_main_get(data);
		if (should_invalidate) {
			KER_action_slot_runtime_users_invalidate(slot, main);
		}
	}
}

void KER_action_slot_runtime_users_add(const ActionSlot *slot, ID *id) {
	ActionSlotRuntime *runtime = static_cast<ActionSlotRuntime *>(slot->runtime);

	ROSE_assert(runtime);

	runtime->users.append_non_duplicates(id);
}

void KER_action_slot_runtime_users_remove(const ActionSlot *slot, ID *id) {
	ActionSlotRuntime *runtime = static_cast<ActionSlotRuntime *>(slot->runtime);

	ROSE_assert(runtime);

	/* Even though users_add() ensures that there are no duplicates, there's still things like
	 * pointer swapping etc. that can happen via the foreach-id looping code. That means that the
	 * entries in the user map are not 100% under control of the user_add() and user_remove()
	 * function, and thus we cannot assume that there are no duplicates. */
	runtime->users.remove_if([&](const ID *user) { return user == id; });
}

void KER_action_slot_runtime_users_invalidate(ActionSlot *slot, Main *main) {
	main->is_action_slot_to_id_map_dirty = true;
}

rose::Span<ID *> KER_action_slot_runtime_users(ActionSlot *slot, Main *main) {
	ActionSlotRuntime *runtime = static_cast<ActionSlotRuntime *>(slot->runtime);

	ROSE_assert(runtime);
	if (main->is_action_slot_to_id_map_dirty) {
		rebuild_slot_user_cache(main);
	}

	return runtime->users;
}

void KER_action_slot_runtime_init(ActionSlot *slot) {
	slot->runtime = MEM_new<ActionSlotRuntime>("ActionSlotRuntime");
}

void KER_action_slot_runtime_free(ActionSlot *slot) {
	ActionSlotRuntime *runtime = static_cast<ActionSlotRuntime *>(slot->runtime);

	if (runtime) {
		MEM_delete<ActionSlotRuntime>(runtime);
	}
}

void KER_action_slot_free(ActionSlot *slot) {
	KER_action_slot_runtime_free(slot);
	MEM_freeN(slot);
}

