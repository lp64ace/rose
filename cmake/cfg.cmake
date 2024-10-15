# -----------------------------------------------------------------------------
# Detect Endianess

include(CheckCSourceCompiles)
include(CheckCSourceRuns)

function(detect_endianess)
	check_c_source_runs("
		#include <stdint.h>

		int main() {
			uint32_t test = 0xdeadbeef;
			uint8_t *p = (uint8_t*)&test;
			return !(p[0] == 0xef && p[1] == 0xbe && p[2] == 0xad&& p[3] == 0xde);
		}
	" LITTLE_ENDIAN)

	if(LITTLE_ENDIAN)
		add_definitions(-D__LITTLE_ENDIAN__)
	endif()

	check_c_source_runs("
		#include <stdint.h>

		int main() {
			uint32_t test = 0xdeadbeef;
			uint8_t *p = (uint8_t*)&test;
			return !(p[0] == 0xde && p[1] == 0xad && p[2] == 0xbe&& p[3] == 0xef);
		}
	" BIG_ENDIAN)

	# Define macro for big-endian if detected
	if(BIG_ENDIAN)
		add_definitions(-D__BIG_ENDIAN__)
	endif()
endfunction()
