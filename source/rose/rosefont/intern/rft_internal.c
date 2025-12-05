#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LIB_utildefines.h"

#include "rft_internal.h"

unsigned int rft_next_p2(unsigned int x) {
	x -= 1;
	x |= (x >> 16);
	x |= (x >> 8);
	x |= (x >> 4);
	x |= (x >> 2);
	x |= (x >> 1);
	x += 1;
	return x;
}

unsigned int rft_hash(unsigned int val) {
	val ^= val >> 13;
	val *= 0x7feb352dU;
	val ^= val >> 15;

	return val & 0xff;
}
