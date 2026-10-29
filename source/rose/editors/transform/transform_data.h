#ifndef TRANSFORM_DATA_H
#define TRANSFORM_DATA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TransDataBase {
	float *loc;
	float iloc[3];
	float center[3];

	int flag;
} TransDataBase;

/** #TransDataBase->flag */
enum {
	TD_SELECTED = 1 << 0,
};

typedef struct TransDataExtension {
	/** Size of the data to transform. */
	float *size;
	/** Initial size before transformation is applied. */
	float isize[3];
} TransDataExtension;

typedef struct TransData {
	TransDataBase base;

	/** Value pointer for special transforms. */
	float *val;
	/** Old value. */
	float ival;
	/** Transformation matrix from data space to global space. */
	float mtx[3][3];
	/** Transformation matrix from global space to data space. */
	float smtx[3][3];
	/** Axis orientation matrix of the data. */
	float axismtx[3][3];

	struct Object *object;
	/** For objects, poses. 1 single allocation per #TransInfo! */
	struct TransDataExtension *extension;
} TransData;

#ifdef __cplusplus
}
#endif

#endif	// !TRANSFORM_DATA_H
