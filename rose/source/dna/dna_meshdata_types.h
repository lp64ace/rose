#pragma once

typedef struct MVert {
	float Coord [ 3 ];
};

typedef struct MLoopUV {
	float TexCoord [ 2 ];
	int Flag;
};

typedef struct MLoopCol {
	unsigned char r , g , b , a;
};

typedef struct MPropCol {
	float color [ 4 ];
} MPropCol;

typedef struct MStringProperty {
	char s [ 255 ] , len;
};
