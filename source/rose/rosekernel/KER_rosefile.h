#ifndef KER_ROSEFILE_H
#define KER_ROSEFILE_H

struct rContext;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RoseFileData {
	struct Main *main;
	struct UserDef *user;
	struct Screen *screen;
	struct Scene *scene;
	struct ViewLayer *view_layer;
} RoseFileData;

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

struct RoseFileData *KER_rosefile_read(const char *filepath, int flag);
void KER_rosefile_read_setup(struct rContext *C, struct RoseFileData *rfd);

/** \} */

#ifdef __cplusplus
}
#endif

#endif	// KER_ROSEFILE_H
