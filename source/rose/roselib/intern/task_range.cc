#include <cstdlib>

#include "MEM_guardedalloc.h"

#include "DNA_listbase.h"

#include "LIB_lazy_threading.hh"
#include "LIB_task.h"
#include "LIB_thread.h"

#include "atomic_ops.h"

void LIB_task_parallel_range(const int start, const int stop, void *userdata, TaskParallelRangeFunc func, const TaskParallelSettings *settings) {
	/* Single threaded. Nothing to reduce as everything is accumulated into the
	 * main userdata chunk directly. */
	TaskParallelTLS tls;
	tls.userdata_chunk = settings->userdata_chunk;
	for (int i = start; i < stop; i++) {
		func(userdata, i, &tls);
	}
	if (settings->func_free != nullptr) {
		settings->func_free(userdata, settings->userdata_chunk);
	}
}

int LIB_task_parallel_thread_id(const TaskParallelTLS * /*tls*/) {
	return 0;
}
