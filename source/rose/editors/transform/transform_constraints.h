#ifndef TRANSFORM_CONSTRAINTS_H
#define TRANSFORM_CONSTRAINTS_H

struct TransInfo;

#ifdef __cplusplus
extern "C" {
#endif

void transform_constraint_start(struct TransInfo *t);
void transform_constraint_stop(struct TransInfo *t);

void transform_constraint_set(struct TransInfo *t, int constraint, const char text[]);

#ifdef __cplusplus
}
#endif

#endif // !TRANSFORM_CONSTRAINTS_H
