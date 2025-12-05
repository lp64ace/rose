#include "LIB_map.hh"
#include "LIB_memarena.h"
#include "LIB_vector.hh"
#include "LIB_vector_set.hh"
#include "LIB_math_axis_angle.hh"
#include "LIB_math_quaternion.hh"

#include "KER_action.h"
#include "KER_action.hh"
#include "KER_anim_data.h"
#include "KER_fcurve.h"
#include "KER_lib_id.h"

#include "fbx_import_anim.hh"

namespace rose::io::fbx {

struct ElementAnimations {
	const ufbx_element *fbx_elem = nullptr;

	const ufbx_anim_prop *prop_position = nullptr;
	const ufbx_anim_prop *prop_rotation = nullptr;
	const ufbx_anim_prop *prop_scale = nullptr;

	ID *id = nullptr;
	int rotmode = ROT_MODE_QUAT;
	size_t order = 0;
};

ROSE_INLINE void set_curve_sample(FCurve *curve, size_t key_index, float time, float value) {
	ROSE_assert(key_index >= 0 && key_index < curve->totvert);
	BezTriple &bez = curve->bezt[key_index];
	bez.vec[1][0] = time;
	bez.vec[1][1] = value;

	// Linear, because it is very important we are "fast as fuck boiiii"

	bez.ipo = BEZT_IPO_LINEAR;
	bez.f1 = bez.f2 = bez.f3 = BEZT_FLAG_SELECT;
	bez.h1 = bez.h2 = HD_AUTO_ANIM;
}

ROSE_INLINE rose::Vector<ElementAnimations> gather_animated_properties(const FbxElementMapping *mapping, const ufbx_anim_layer *flayer) {
	size_t order = 0;
	rose::Map<const ufbx_element *, ElementAnimations> elem_map;
	for (const ufbx_anim_prop &fprop : flayer->anim_props) {
		if (fprop.anim_value->curves[0] == nullptr) {
			continue;
		}

		bool supported_prop = false;

		const bool is_position = STREQ(fprop.prop_name.data, "Lcl Translation");
		const bool is_rotation = STREQ(fprop.prop_name.data, "Lcl Rotation");
		const bool is_scale = STREQ(fprop.prop_name.data, "Lcl Scaling");

		if (is_position || is_rotation || is_scale) {
			supported_prop = true;
		}

		if (!supported_prop) {
			continue;
		}

		ID *id = nullptr;
		int rotmode = ROT_MODE_QUAT;

		if (true) {
			const ufbx_node *fnode = ufbx_as_node(fprop.element);

			Object *object = nullptr;

			if (fnode) {
				object = mapping->bone_to_armature.lookup_default(fnode, nullptr);
			}
			if (object == NULL) {
				object = mapping->el_to_object.lookup_default(fprop.element, nullptr);
			}
			if (object == NULL) {
				continue;
			}

			if (object->type == OB_MESH && object->parent && object->parent->type == OB_ARMATURE) {
				continue;
			}

			id = &object->id;
			rotmode = object->rotmode;
		}

		if (id == NULL) {
			continue;
		}

		ElementAnimations &anim = elem_map.lookup_or_add_default(fprop.element);
		anim.fbx_elem = fprop.element;
		anim.order = order++;
		anim.id = id;
		anim.rotmode = rotmode;

		if (is_position) {
			anim.prop_position = &fprop;
		}
		if (is_rotation) {
			anim.prop_rotation = &fprop;
		}
		if (is_scale) {
			anim.prop_scale = &fprop;
		}
	}


	/* Sort returned result in the original fbx file order. */
	rose::Vector<ElementAnimations> animations(elem_map.values().begin(), elem_map.values().end());
	std::sort(animations.begin(), animations.end(), [](const ElementAnimations &a, const ElementAnimations &b) {
		return a.order < b.order;
	});
	return animations;
}

void create_transform_curve_desc(FbxElementMapping *mapping, const ElementAnimations *anim, MemArena *names, Vector<FCurveDescriptor>& descr) {
	/* For animated bones, prepend bone path to animation curve path. */
	std::string prefix;
	std::string group = get_fbx_name(anim->fbx_elem->name, "");
	const ufbx_node *fnode = ufbx_as_node(anim->fbx_elem);
	const bool is_bone = mapping->node_is_rose_bone.contains(fnode);
	if (is_bone) {
		group = mapping->node_to_name.lookup_default(fnode, "");
		prefix = std::string("pose.bones[\"") + group + "\"].";
	}

	const char *groupname = static_cast<const char *>(LIB_memory_arena_strdup(names, &group[0]));
	const char *rnaposition = static_cast<const char *>(LIB_memory_arena_strfmt(names, "%slocation", &prefix[0]));
	const char *rnarotation = NULL;

	int rotchannels = 3;

	int rotmode = is_bone ? ROT_MODE_QUAT : anim->rotmode;
	switch (rotmode) {
		case ROT_MODE_QUAT: {
			rnarotation = static_cast<const char *>(LIB_memory_arena_strfmt(names, "%squaternion", &prefix[0]));
			rotchannels = 4;
		} break;
		case ROT_MODE_AXISANGLE: {
			rnarotation = static_cast<const char *>(LIB_memory_arena_strfmt(names, "%saxis_angle", &prefix[0]));
			rotchannels = 4;
		} break;
		default: {
			rnarotation = static_cast<const char *>(LIB_memory_arena_strfmt(names, "%seuler", &prefix[0]));
			rotchannels = 3;
		} break;
	}
	
	const char *rnascale = static_cast<const char *>(LIB_memory_arena_strfmt(names, "%sscale", &prefix[0]));

	for (size_t i = 0; i < 3; i++) {
		FCurveDescriptor curve;
		
		curve.path = rnaposition;
		curve.index = i;
		curve.type = -1;
		curve.subtype = -1;
		curve.group = groupname;

		descr.append(curve);
	}
	for (size_t i = 0; i < rotchannels; i++) {
		FCurveDescriptor curve;

		curve.path = rnarotation;
		curve.index = i;
		curve.type = -1;
		curve.subtype = -1;
		curve.group = groupname;

		descr.append(curve);
	}
	for (size_t i = 0; i < 3; i++) {
		FCurveDescriptor curve;

		curve.path = rnascale;
		curve.index = i;
		curve.type = -1;
		curve.subtype = -1;
		curve.group = groupname;

		descr.append(curve);
	}
}

ROSE_STATIC double create_transform_curve_data(const FbxElementMapping *mapping, const ufbx_anim *fanim, const ElementAnimations *anim, const double fps, FCurve **fcurves) {
	const ufbx_node *fnode = ufbx_as_node(anim->fbx_elem);
	ufbx_matrix bone_xform = ufbx_identity_matrix;
	const bool is_bone = mapping->node_is_rose_bone.contains(fnode);
	if (is_bone) {
		/* Bone transform curves need to be transformed to the bind transform
		 * in joint-local space:
		 * - Calculate local space bind matrix: inv(parent_bind) * bind
		 * - Invert the result; this will be used to transform loc/rot/scale curves. */

		const bool bone_at_scene_root = fnode->node_depth <= 1;
		ufbx_matrix world_to_arm = ufbx_identity_matrix;
		if (!bone_at_scene_root) {
			Object *arm_obj = mapping->bone_to_armature.lookup_default(fnode, nullptr);
			if (arm_obj != nullptr) {
				world_to_arm = mapping->armature_world_to_arm_pose_matrix.lookup_default(arm_obj, ufbx_identity_matrix);
			}
		}

		bone_xform = mapping->calc_local_bind_matrix(fnode, world_to_arm);
		bone_xform = ufbx_matrix_invert(&bone_xform);
	}

	int rotchannels = 3;

	int rotmode = is_bone ? ROT_MODE_QUAT : anim->rotmode;
	switch (rotmode) {
		case ROT_MODE_QUAT: {
			rotchannels = 4;
		} break;
		case ROT_MODE_AXISANGLE: {
			rotchannels = 4;
		} break;
		default: {
			rotchannels = 3;
		} break;
	}

	/* Note: Python importer was always creating all pos/rot/scale curves: "due to all FBX
	 * transform magic, we need to add curves for whole loc/rot/scale in any case".
	 *
	 * Also, we create a full transform keyframe at any point where input pos/rot/scale curves have
	 * a keyframe. It should not be needed if we fully imported curves with all their proper
	 * handles, but again currently this is to match Python importer behavior. */
	const ufbx_anim_curve *input_curves[9] = {};
	if (anim->prop_position) {
		input_curves[0] = anim->prop_position->anim_value->curves[0];
		input_curves[1] = anim->prop_position->anim_value->curves[1];
		input_curves[2] = anim->prop_position->anim_value->curves[2];
	}
	if (anim->prop_rotation) {
		input_curves[3] = anim->prop_rotation->anim_value->curves[0];
		input_curves[4] = anim->prop_rotation->anim_value->curves[1];
		input_curves[5] = anim->prop_rotation->anim_value->curves[2];
	}
	if (anim->prop_scale) {
		input_curves[6] = anim->prop_scale->anim_value->curves[0];
		input_curves[7] = anim->prop_scale->anim_value->curves[1];
		input_curves[8] = anim->prop_scale->anim_value->curves[2];
	}

	/* Figure out timestamps of where any of input curves have a keyframe. */
	Set<double> unique_key_times;
	for (int i = 0; i < 9; i++) {
		if (input_curves[i] != nullptr) {
			for (const ufbx_keyframe &key : input_curves[i]->keyframes) {
				if (key.interpolation == UFBX_INTERPOLATION_CUBIC) {
					/* Hack: force cubic keyframes to be linear, to match Python importer behavior. */
					const_cast<ufbx_keyframe &>(key).interpolation = UFBX_INTERPOLATION_LINEAR;
				}
				unique_key_times.add(key.time);
			}
		}
	}
	Vector<double> sorted_key_times(unique_key_times.begin(), unique_key_times.end());
	std::sort(sorted_key_times.begin(), sorted_key_times.end());

	size_t posindex = 0;
	size_t rotindex = posindex + 3;
	size_t sclindex = rotindex + rotchannels;
	size_t totcurve = sclindex + 3;
	for (size_t i = 0; i < totcurve; i++) {
		ROSE_assert_msg(fcurves[i], "fbx: animation curve was not created successfully");
		KER_fcurve_bezt_resize(fcurves[i], sorted_key_times.size());
	}

	/* Evaluate transforms at all the key times. */
	math::Quaternion quat_prev = math::Quaternion::identity();
	for (size_t i = 0; i < sorted_key_times.size(); i++) {
		double t = sorted_key_times[i];
		float tf = float(t * fps);
		ufbx_transform xform = ufbx_evaluate_transform(fanim, fnode, t);

		if (is_bone) {
			ufbx_matrix matrix = calc_bone_pose_matrix(xform, *fnode, bone_xform);
			xform = ufbx_matrix_to_transform(&matrix);
		}

		set_curve_sample(fcurves[posindex + 0], i, tf, float(xform.translation.x));
		set_curve_sample(fcurves[posindex + 1], i, tf, float(xform.translation.y));
		set_curve_sample(fcurves[posindex + 2], i, tf, float(xform.translation.z));

		math::Quaternion quat(xform.rotation.w, xform.rotation.x, xform.rotation.y, xform.rotation.z);
		switch (rotmode) {
			case ROT_MODE_QUAT:
				/* Ensure shortest interpolation path between consecutive quaternions. */
				if (i != 0 && math::dot(quat, quat_prev) < 0.0f) {
					quat = -quat;
				}
				quat_prev = quat;
				set_curve_sample(fcurves[rotindex + 0], i, tf, quat.w);
				set_curve_sample(fcurves[rotindex + 1], i, tf, quat.x);
				set_curve_sample(fcurves[rotindex + 2], i, tf, quat.y);
				set_curve_sample(fcurves[rotindex + 3], i, tf, quat.z);
				break;
			case ROT_MODE_AXISANGLE: {
				const math::AxisAngle axis_angle = math::to_axis_angle(quat);
				set_curve_sample(fcurves[rotindex + 0], i, tf, axis_angle.angle().radian());
				set_curve_sample(fcurves[rotindex + 1], i, tf, axis_angle.axis().x);
				set_curve_sample(fcurves[rotindex + 2], i, tf, axis_angle.axis().y);
				set_curve_sample(fcurves[rotindex + 3], i, tf, axis_angle.axis().z);
			} break;
			default: {
				math::EulerXYZ euler = math::to_euler(quat);
				set_curve_sample(fcurves[rotindex + 0], i, tf, euler.x().radian());
				set_curve_sample(fcurves[rotindex + 1], i, tf, euler.y().radian());
				set_curve_sample(fcurves[rotindex + 2], i, tf, euler.z().radian());
			} break;
		}

		set_curve_sample(fcurves[sclindex + 0], i, tf, float(xform.scale.x));
		set_curve_sample(fcurves[sclindex + 1], i, tf, float(xform.scale.y));
		set_curve_sample(fcurves[sclindex + 2], i, tf, float(xform.scale.z));
	}

	return sorted_key_times.last() * fps;
}

void import_animations(Main *main, Scene *scene, const ufbx_scene *fbx, FbxElementMapping *mapping, const double fps) {
	for (const ufbx_anim_stack *fstack : fbx->anim_stacks) {
		for (const ufbx_anim_layer *flayer : fstack->layers) {
			rose::Vector<ElementAnimations> animations = gather_animated_properties(mapping, flayer);
			if (animations.is_empty()) {
				continue;
			}

			/* Create action for this layer. */
			std::string action_name = fstack->name.data;
			if (!STREQ(fstack->name.data, flayer->name.data) && fstack->layers.count != 1) {
				action_name += '|';
				action_name += flayer->name.data;
			}

			Action *action = static_cast<Action *>(KER_id_new(main, ID_AC, &action_name[0]));
			KER_action_keystrip_ensure(action);

			ActionLayer *layer = action->layers[0];
			ActionStrip *strip = layer->strips[0];
			ActionStripKeyframeData *strip_data = KER_action_strip_data(action, strip);

			rose::VectorSet<ID *> ids;
			rose::Map<ID *, rose::Vector<const ElementAnimations *>> id_to_anims;

			for (const ElementAnimations &animation : animations) {
				ids.add(animation.id);

				rose::Vector<const ElementAnimations *> &anims = id_to_anims.lookup_or_add_default(animation.id);
				anims.append(&animation);
			}

			for (ID *id : ids) {
				ROSE_assert(id);

				ActionSlot *slot = KER_action_slot_add_for_idtype(action, GS(id->name));
				KER_action_slot_identifier_define(action, slot, id->name + 2);

				const AnimData *adt = KER_animdata_ensure_id(id);
				if (adt->action == NULL) {
					bool ok = KER_action_assign(action, id);
					ROSE_assert_msg(ok, "[IOFbx] Could not assign action to ID");
					UNUSED_VARS_NDEBUG(ok);
				}
				if (!adt->handle) {
					bool ok = KER_action_slot_assign(slot, id);
					ROSE_assert_msg(ok, "[IOFbx] Could not assign action slot to ID");
					UNUSED_VARS_NDEBUG(ok);
				}

				ActionChannelBag *channelbag = KER_action_strip_keyframe_data_ensure_channelbag_for_slot(strip_data, slot);

				rose::Vector<const ElementAnimations *> id_anims = id_to_anims.lookup(id);
				
				/**
				 * Batch create the transform curves: creating them one by one is not very fast,
				 * especially for armatures where many bones often are animated. So first create
				 * their descriptors, then create the f-curves in one step, and finally fill their data.
				 */
				Vector<FCurveDescriptor> curve_desc;
				rose::Vector<size_t> anim_transform_curve_index(id_anims.size());

				MemArena *names = LIB_memory_arena_create(1024, "CurveNameAllocator");
				for (const size_t index : id_anims.index_range()) {
					const ElementAnimations *anim = id_anims[index];
					if (anim->prop_position || anim->prop_rotation || anim->prop_scale) {
						std::string group_name_str = get_fbx_name(anim->fbx_elem->name, "");
						anim_transform_curve_index[index] = curve_desc.size();
						create_transform_curve_desc(mapping, anim, names, curve_desc);
					}
					else {
						anim_transform_curve_index[index] = -1;
					}
				}

				rose::Vector<FCurve *> fcurves(curve_desc.size());
				if (!curve_desc.is_empty()) {
					KER_action_channelbag_fcurve_create_many(NULL, channelbag, curve_desc.data(), curve_desc.size(), fcurves.data());
				}
				
				for (const size_t index : id_anims.index_range()) {
					const ElementAnimations *anim = id_anims[index];
					if (anim->prop_position || anim->prop_rotation || anim->prop_scale) {
						float duration = create_transform_curve_data(mapping, flayer->anim, anim, fps, fcurves.data() + anim_transform_curve_index[index]);

						action->frame_start = 0;
						action->frame_end = duration;
					}
				}

				fprintf(stdout, "Action[%s:%.1f]\n", action->id.name, action->frame_end);

				for (FCurve *curve : fcurves) {
					KER_fcurve_handles_recalc(curve);
				}

				LIB_memory_arena_destroy(names);
			}
		}
	}
}

}  // namespace rose::io::fbx
