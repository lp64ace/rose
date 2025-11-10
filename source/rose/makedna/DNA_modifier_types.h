#ifndef KER_MODIFIER_TYPES_H
#define KER_MODIFIER_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ModifierData {
	struct ModifierData *prev, *next;

	int type;
	int flag;

	char name[64];

	/**
	 * Runtime field which contains runtime data which is specified to a modifier type.
	 */
	void *runtime;
} ModifierData;

typedef enum eModifierType {
	MODIFIER_TYPE_NONE = 0,
	MODIFIER_TYPE_ARMATURE = 1,
	NUM_MODIFIER_TYPES,
} eModifierType;

typedef enum eModifierFlag {
	MODIFIER_DEFORM_DIRTY = 1 << 0,
} eModifierFlag;

/* -------------------------------------------------------------------- */
/** \name Armature Modifier Data
 * \{ */

typedef struct ArmatureModifierData {
	ModifierData modifier;

	struct Object *object;
} ArmatureModifierData;

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_MODIFIER_TYPES_H
