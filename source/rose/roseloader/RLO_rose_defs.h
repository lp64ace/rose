#pragma once

#include "LIB_endian.h"
#include "LIB_utildefines.h"

enum {
	/**
	 * Arbitrary allocated memory.
	 * (typically owned by #ID's, will be freed when there are no users).
	 */
	RLO_CODE_DATA = MAKEID4('D', 'A', 'T', 'A'),
	/**
	 * Used for storing the encoded SDNA string, 
	 * we do not use 'S', 'D', 'N', 'A' here since this is used to detect endian later on when #SDNA is loaded
	 * (decoded into an #SDNA on load).
	 */
	RLO_CODE_DNA1 = MAKEID4('D', 'N', 'A', '1'),
	/**
	 * Terminate reading (no data),
	 */
	RLO_CODE_ENDB = MAKEID4('E', 'N', 'D', 'B'),
};
