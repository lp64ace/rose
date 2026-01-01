#ifndef KER_ANIM_SYS_H
#define KER_ANIM_SYS_H

struct AnimData;
struct ID;
struct FCurve;
struct PathResolvedRNA;
struct PointerRNA;
struct Scene;

#ifdef __cplusplus
extern "C" {
#endif

enum {
	ADT_RECALC_ANIM = (1 << 0),
	ADT_RECALC_ALL = (ADT_RECALC_ANIM),
};

void KER_animsys_evaluate_animdata(struct ID *id, struct AnimData *adt, float time, int recalc);
void KER_animsys_eval_animdata(struct Scene *scene, struct ID *id);

bool KER_animsys_rna_path_resolve(struct PointerRNA *ptr, const char *path, int index, struct PathResolvedRNA *result);
bool KER_animsys_rna_curve_resolve(struct PointerRNA *ptr, struct FCurve *fcurve, struct PathResolvedRNA *result);

#ifdef __cplusplus
}
#endif

#endif
