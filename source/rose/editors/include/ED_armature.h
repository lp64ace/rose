#ifndef ED_ARMATURE_H
#define ED_ARMATURE_H

struct Armature;
struct EditBone;
struct Main;

#ifdef __cplusplus
extern "C" {
#endif
	
/** Convert the current armature into edit-mode. */
void ED_armature_edit_new(struct Armature *armature);
void ED_armature_edit_apply(struct Main *main, struct Armature *armature);
void ED_armature_edit_free(struct Armature *armature);

struct EditBone *ED_armature_ebone_add(struct Armature *armature, const char *name);

void ED_armature_ebone_to_mat3(struct EditBone *ebone, float r_mat[3][3]);
void ED_armature_ebone_to_mat4(struct EditBone *ebone, float r_mat[4][4]);
void ED_armature_ebone_from_mat3(struct EditBone *ebone, const float mat[3][3]);
void ED_armature_ebone_from_mat4(struct EditBone *ebone, const float mat[4][4]);

#ifdef __cplusplus
}
#endif

#endif
