#ifndef DNA_CURVE_TYPES_H
#define DNA_CURVE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BezTriple {
	float vec[3][3];

	unsigned char h1, h2;
	unsigned char f1, f2, f3;

	unsigned char type;
	unsigned char ipo;
	unsigned char easing;

	float back;
	float amptiture;
	float period;
} BezTriple;

/* #BezTriple->f1, #BezTriple->f2, #BezTriple->f3 */
enum {
	BEZT_FLAG_SELECT = (1 << 0),
};

/* #BezTriple->h1, #BezTriple->h2 */
enum {
	HD_FREE = 0,
	HD_AUTO,
	HD_ALIGN,
	HD_AUTO_ANIM,
	HD_ALIGN_DOUBLESIDE,
};

#define BEZT_IS_AUTOH(bezt) (ELEM((bezt)->h1, HD_AUTO, HD_AUTO_ANIM) && ELEM((bezt)->h2, HD_AUTO, HD_AUTO_ANIM))

/** #BezTriple->type */
enum {
	/* Normal automatic handle that can be refined further. */
	HD_AUTOTYPE_NORMAL = 0,
	/**
	 * Handle locked horizontal due to being an Auto Clamped local
	 * extreme or a curve endpoint with Constant extrapolation.
	 * Further smoothing is disabled.
	 */
	HD_AUTOTYPE_LOCKED_FINAL = 1,
};

/** #BezTriple->ipo */
enum {
	BEZT_IPO_CONST = 0,
	BEZT_IPO_LINEAR,
	BEZT_IPO_BEZ,

	// BEZT_IPO_BACK,
	// BEZT_IPO_BOUNCE,
	// BEZT_IPO_CIRC,
	// BEZT_IPO_CUBIC,
	// BEZT_IPO_EXPO,
	// BEZT_IPO_QUAD,
	// BEZT_IPO_QUART,
	// BEZT_IPO_QUINT,
	// BEZT_IPO_SINE,
};

enum {
	BEZT_EASE_AUTO = 0,

	BEZT_EASE_IN,
	BEZT_EASE_OUT,
	BEZT_EASE_IN_OUT,
};

#ifdef __cplusplus
}
#endif

#endif /* DNA_CURVE_TYPES_H */