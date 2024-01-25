#include "MEM_alloc.h"

#include "LIB_assert.h"
#include "LIB_string.h"
#include "LIB_system.h"

void log(const char *str, size_t indicator) {
	size_t i;
	for(i = 0; str[i] != '\0'; i++) {
		printf("%c", str[i]);
	}
	printf("\n");
	for(i = 0; i < indicator && str[i] != '\0'; i++) {
		printf(" ");
	}
	printf("^\n");
}

int main(void) {
	MEM_use_memleak_detection(true);
	MEM_enable_fail_on_memleak();
	MEM_init_memleak_detection();
	
	LIB_system_signal_callbacks_init();

	return 0;
}
