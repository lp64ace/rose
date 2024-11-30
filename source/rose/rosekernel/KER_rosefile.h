#ifndef KER_ROSEFILE_H
#define KER_ROSEFILE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RoseFileData {
	struct Main *main;
	struct UserDef *user;
} RoseFileData;

/* -------------------------------------------------------------------- */
/** \name Util Methods
 * \{ */

void KER_rosefile_read_setup(RoseFileData *rfd);

/** \} */

#ifdef __cplusplus
}
#endif

#endif // KER_ROSEFILE_H
