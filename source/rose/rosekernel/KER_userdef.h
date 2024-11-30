#ifndef KER_USERDEF_H
#define KER_USERDEF_H

#ifdef __cplusplus
extern "C" {
#endif

struct UserDef;

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void KER_rose_userdef_data_set_and_free(struct UserDef *userdef);
void KER_userdef_clear(struct UserDef *userdef);
void KER_userdef_free(struct UserDef *userdef);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_USERDEF_H