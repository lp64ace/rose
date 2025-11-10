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

enum {
	/**
	 * Used when we do not want the vertex deformation take place on the
	 * host (CPU) but rather on the GPU and with no intention of copying
	 * the result to the host (CPU), this makes the #draw module the one
	 * responsible for applying the modifier!
	 * 
	 * Do NOT mark multiple modifiers of the same type for device only 
	 * usage since only the last one will be applied!
	 */
	MODIFIER_DEVICE_ONLY = (1 << 0),
};

typedef enum eModifierType {
	MODIFIER_TYPE_NONE = 0,
	MODIFIER_TYPE_ARMATURE = 1,
	NUM_MODIFIER_TYPES,
} eModifierType;

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
