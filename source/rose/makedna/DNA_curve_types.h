#ifndef DNA_CURVE_TYPES_H
#define DNA_CURVE_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BezTriple {
	float vec[3][3];

	unsigned char h1, h2;
	unsigned char f1, f2, f3;

	unsigned char ipo;
	unsigned char easing;

	float back;
	float amptiture;
	float period;
} BezTriple;

/** #BezTriple->ipo */
enum {
	BEZT_IPO_CONST = 0,
	BEZT_IPO_LINEAR,
	BEZT_IPO_BEZ,

	BEZT_IPO_BACK,
	BEZT_IPO_BOUNCE,
	BEZT_IPO_CIRC,
	BEZT_IPO_CUBIC,
	BEZT_IPO_ELASTIC,
	BEZT_IPO_EXPO,
	BEZT_IPO_QUAD,
	BEZT_IPO_QUART,
	BEZT_IPO_QUINT,
	BEZT_IPO_SINE,
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