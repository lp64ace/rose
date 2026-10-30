#ifndef TRANSFORM_ORIENTATIONS_H
#define TRANSFORM_ORIENTATIONS_H

struct TransInfo;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Those two fill in mat and return non-zero on success.
 */
bool transform_orientations_create_from_axis(float mat[3][3], const float x[3], const float y[3], const float z[3]);

/** Set the current t->spacemat according to the type of orientation. */
void transform_orientations_current_set(struct TransInfo *t);

#ifdef __cplusplus
}
#endif

#endif // !TRANSFORM_ORIENTATIONS_H
