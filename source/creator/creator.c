#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_system.h"

int main(void) {
	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();
	
	LIB_system_signal_callbacks_init();

	const char *haystack = "willbeverylongtooverflowhash edge case for beggining Something that willbeverylongtooverflowhash blah blah something kys nigger something that willbeverylongtooverflowhash", *needle = "willbeverylongtooverflowhash";
	while(LIB_strfind(haystack, needle) != LIB_NPOS) {
		size_t i, index = LIB_strfind(haystack, needle);
		printf("%s\n", haystack);
		for(i = 0; i < index; i++) {
			printf(" ");
		}
		printf("^\n");
		
		haystack = haystack + index + 1;
	}

	return 0;
}
