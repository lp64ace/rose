#ifndef TRANSFORM_ORIENTATIONS_H
#define TRANSFORM_ORIENTATIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Those two fill in mat and return non-zero on success.
 */
bool transform_orientations_create_from_axis(float mat[3][3], const float x[3], const float y[3], const float z[3]);

#ifdef __cplusplus
}
#endif

#endif // !TRANSFORM_ORIENTATIONS_H
