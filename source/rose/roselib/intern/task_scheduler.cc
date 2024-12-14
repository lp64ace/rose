#include "MEM_guardedalloc.h"

#include "LIB_lazy_threading.hh"
#include "LIB_task.h"
#include "LIB_thread.h"

/* Task Scheduler */

static int task_scheduler_num_threads = 1;

void LIB_task_scheduler_init() {
	task_scheduler_num_threads = LIB_system_thread_count();
}

void LIB_task_scheduler_exit() {
}

int LIB_task_scheduler_num_threads() {
	return task_scheduler_num_threads;
}

void LIB_task_isolate(void (*func)(void *userdata), void *userdata) {
	func(userdata);
}
