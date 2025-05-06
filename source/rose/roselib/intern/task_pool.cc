#include <cstdlib>
#include <memory>
#include <utility>

#include "MEM_guardedalloc.h"

#include "DNA_listbase.h"

#include "LIB_gsqueue.h"
#include "LIB_math_base.h"
#include "LIB_math_bit.h"
#include "LIB_mempool.h"
#include "LIB_task.h"
#include "LIB_thread.h"

/* Task
 *
 * Unit of work to execute. This is a C++ class to work with TBB. */

class Task {
public:
	TaskPool *pool;
	TaskRunFunction run;
	void *taskdata;
	bool free_taskdata;
	TaskFreeFunction freedata;

	Task(TaskPool *pool, TaskRunFunction run, void *taskdata, bool free_taskdata, TaskFreeFunction freedata) : pool(pool), run(run), taskdata(taskdata), free_taskdata(free_taskdata), freedata(freedata) {
	}

	~Task() {
		if (free_taskdata) {
			if (freedata) {
				freedata(pool, taskdata);
			}
			else {
				MEM_freeN(taskdata);
			}
		}
	}

	/* Move constructor.
	 * For performance, ensure we never copy the task and only move it.
	 * For TBB version 2017 and earlier we apply a workaround to make up for
	 * the lack of move constructor support. */
	Task(Task &&other) : pool(other.pool), run(other.run), taskdata(other.taskdata), free_taskdata(other.free_taskdata), freedata(other.freedata) {
		other.pool = nullptr;
		other.run = nullptr;
		other.taskdata = nullptr;
		other.free_taskdata = false;
		other.freedata = nullptr;
	}

	Task(const Task &other) = delete;

	Task &operator=(const Task &other) = delete;
	Task &operator=(Task &&other) = delete;

	void operator()() const;
};

/* Task Pool */

enum TaskPoolType {
	TASK_POOL_TBB,
	TASK_POOL_TBB_SUSPENDED,
	TASK_POOL_NO_THREADS,
	TASK_POOL_BACKGROUND,
	TASK_POOL_BACKGROUND_SERIAL,
};

struct TaskPool {
	TaskPoolType type;
	bool use_threads;

	ThreadMutex user_mutex;
	void *userdata;

	volatile bool is_suspended;
	MemPool *suspended_mempool;

	/* Background task pool. */
	ListBase background_threads;
	ThreadQueue *background_queue;
	volatile bool background_is_canceling;
};

/* Execute task. */
void Task::operator()() const {
	run(pool, taskdata);
}

/* TBB Task Pool.
 *
 * Task pool using the TBB scheduler for tasks. When building without TBB
 * support or running Blender with -t 1, this reverts to single threaded.
 *
 * Tasks may be suspended until in all are created, to make it possible to
 * initialize data structures and create tasks in a single pass. */

static void tbb_task_pool_create(TaskPool *pool, eTaskPriority priority) {
	if (pool->type == TASK_POOL_TBB_SUSPENDED) {
		pool->is_suspended = true;
		pool->suspended_mempool = LIB_memory_pool_create(sizeof(Task), 512, 512, ROSE_MEMPOOL_ALLOW_ITER);
	}

	EXPR_NOP(priority);
}

static void tbb_task_pool_run(TaskPool *pool, Task &&task) {
	if (pool->is_suspended) {
		/* Suspended task that will be executed in work_and_wait(). */
		Task *task_mem = (Task *)LIB_memory_pool_malloc(pool->suspended_mempool);
		new (task_mem) Task(std::move(task));
#ifdef __GNUC__
		/* Work around apparent compiler bug where task is not properly copied
		 * to task_mem. This appears unrelated to the use of placement new or
		 * move semantics, happens even writing to a plain C struct. Rather the
		 * call into TBB seems to have some indirect effect. */
		std::atomic_thread_fence(std::memory_order_release);
#endif
	}
	else {
		/* Execute immediately. */
		task();
	}
}

static void tbb_task_pool_work_and_wait(TaskPool *pool) {
	/* Start any suspended task now. */
	if (pool->suspended_mempool) {
		pool->is_suspended = false;

		MemPoolIter iter;
		LIB_memory_pool_iternew(pool->suspended_mempool, &iter);
		while (Task *task = (Task *)LIB_memory_pool_iterstep(&iter)) {
			tbb_task_pool_run(pool, std::move(*task));
		}

		LIB_memory_pool_clear(pool->suspended_mempool, 0);
	}
}

static void tbb_task_pool_cancel(TaskPool *pool) {
	EXPR_NOP(pool);
}

static bool tbb_task_pool_canceled(TaskPool *pool) {
	EXPR_NOP(pool);

	return false;
}

static void tbb_task_pool_free(TaskPool *pool) {
	if (pool->suspended_mempool) {
		LIB_memory_pool_destroy(pool->suspended_mempool);
	}
}

/* Background Task Pool.
 *
 * Fallback for running background tasks when building without TBB. */

static void *background_task_run(void *userdata) {
	TaskPool *pool = (TaskPool *)userdata;
	while (Task *task = (Task *)LIB_thread_queue_pop(pool->background_queue)) {
		(*task)();
		task->~Task();
		MEM_freeN(task);
	}
	return nullptr;
}

static void background_task_pool_create(TaskPool *pool) {
	pool->background_queue = LIB_thread_queue_init();
	LIB_threadpool_init(&pool->background_threads, background_task_run, 1);
}

static void background_task_pool_run(TaskPool *pool, Task &&task) {
	Task *task_mem = (Task *)MEM_mallocN(sizeof(Task), __func__);
	new (task_mem) Task(std::move(task));
	LIB_thread_queue_push(pool->background_queue, task_mem);

	if (LIB_available_threads(&pool->background_threads)) {
		LIB_threadpool_insert(&pool->background_threads, pool);
	}
}

static void background_task_pool_work_and_wait(TaskPool *pool) {
	/* Signal background thread to stop waiting for new tasks if none are
	 * left, and wait for tasks and thread to finish. */
	LIB_thread_queue_nowait(pool->background_queue);
	LIB_thread_queue_wait_finish(pool->background_queue);
	LIB_threadpool_clear(&pool->background_threads);
}

static void background_task_pool_cancel(TaskPool *pool) {
	pool->background_is_canceling = true;

	/* Remove tasks not yet started by background thread. */
	LIB_thread_queue_nowait(pool->background_queue);
	while (Task *task = (Task *)LIB_thread_queue_pop(pool->background_queue)) {
		task->~Task();
		MEM_freeN(task);
	}

	/* Let background thread finish or cancel task it is working on. */
	LIB_threadpool_remove(&pool->background_threads, pool);
	pool->background_is_canceling = false;
}

static bool background_task_pool_canceled(TaskPool *pool) {
	return pool->background_is_canceling;
}

static void background_task_pool_free(TaskPool *pool) {
	background_task_pool_work_and_wait(pool);

	LIB_threadpool_end(&pool->background_threads);
	LIB_thread_queue_free(pool->background_queue);
}

/* Task Pool */

static TaskPool *task_pool_create_ex(void *userdata, TaskPoolType type, eTaskPriority priority) {
	const bool use_threads = LIB_task_scheduler_num_threads() > 1 && type != TASK_POOL_NO_THREADS;

	/* Background task pool uses regular TBB scheduling if available. Only when
	 * building without TBB or running with -t 1 do we need to ensure these tasks
	 * do not block the main thread. */
	if (type == TASK_POOL_BACKGROUND && use_threads) {
		type = TASK_POOL_TBB;
	}

	/* Allocate task pool. */
	TaskPool *pool = (TaskPool *)MEM_callocN(sizeof(TaskPool), "TaskPool");

	pool->type = type;
	pool->use_threads = use_threads;

	pool->userdata = userdata;
	LIB_mutex_init(&pool->user_mutex);

	switch (type) {
		case TASK_POOL_TBB:
		case TASK_POOL_TBB_SUSPENDED:
		case TASK_POOL_NO_THREADS:
			tbb_task_pool_create(pool, priority);
			break;
		case TASK_POOL_BACKGROUND:
		case TASK_POOL_BACKGROUND_SERIAL:
			background_task_pool_create(pool);
			break;
	}

	return pool;
}

/**
 * Create a normal task pool. Tasks will be executed as soon as they are added.
 */
TaskPool *LIB_task_pool_create(void *userdata, eTaskPriority priority) {
	return task_pool_create_ex(userdata, TASK_POOL_TBB, priority);
}

/**
 * Create a background task pool.
 * In multi-threaded context, there is no differences with #LIB_task_pool_create(),
 * but in single-threaded case it is ensured to have at least one worker thread to run on
 * (i.e. you don't have to call #LIB_task_pool_work_and_wait
 * on it to be sure it will be processed).
 *
 * \note Background pools are non-recursive
 * (that is, you should not create other background pools in tasks assigned to a background pool,
 * they could end never being executed, since the 'fallback' background thread is already
 * busy with parent task in single-threaded context).
 */
TaskPool *LIB_task_pool_create_background(void *userdata, eTaskPriority priority) {
	return task_pool_create_ex(userdata, TASK_POOL_BACKGROUND, priority);
}

/**
 * Similar to LIB_task_pool_create() but does not schedule any tasks for execution
 * for until LIB_task_pool_work_and_wait() is called. This helps reducing threading
 * overhead when pushing huge amount of small initial tasks from the main thread.
 */
TaskPool *LIB_task_pool_create_suspended(void *userdata, eTaskPriority priority) {
	return task_pool_create_ex(userdata, TASK_POOL_TBB_SUSPENDED, priority);
}

/**
 * Single threaded task pool that executes pushed task immediately, for
 * debugging purposes.
 */
TaskPool *LIB_task_pool_create_no_threads(void *userdata) {
	return task_pool_create_ex(userdata, TASK_POOL_NO_THREADS, TASK_PRIORITY_HIGH);
}

/**
 * Task pool that executes one task after the other, possibly on different threads
 * but never in parallel.
 */
TaskPool *LIB_task_pool_create_background_serial(void *userdata, eTaskPriority priority) {
	return task_pool_create_ex(userdata, TASK_POOL_BACKGROUND_SERIAL, priority);
}

void LIB_task_pool_free(TaskPool *pool) {
	switch (pool->type) {
		case TASK_POOL_TBB:
		case TASK_POOL_TBB_SUSPENDED:
		case TASK_POOL_NO_THREADS:
			tbb_task_pool_free(pool);
			break;
		case TASK_POOL_BACKGROUND:
		case TASK_POOL_BACKGROUND_SERIAL:
			background_task_pool_free(pool);
			break;
	}

	LIB_mutex_end(&pool->user_mutex);

	MEM_freeN(pool);
}

void LIB_task_pool_push(TaskPool *pool, TaskRunFunction run, void *taskdata, bool free_taskdata, TaskFreeFunction freedata) {
	Task task(pool, run, taskdata, free_taskdata, freedata);

	switch (pool->type) {
		case TASK_POOL_TBB:
		case TASK_POOL_TBB_SUSPENDED:
		case TASK_POOL_NO_THREADS:
			tbb_task_pool_run(pool, std::move(task));
			break;
		case TASK_POOL_BACKGROUND:
		case TASK_POOL_BACKGROUND_SERIAL:
			background_task_pool_run(pool, std::move(task));
			break;
	}
}

void LIB_task_pool_work_and_wait(TaskPool *pool) {
	switch (pool->type) {
		case TASK_POOL_TBB:
		case TASK_POOL_TBB_SUSPENDED:
		case TASK_POOL_NO_THREADS:
			tbb_task_pool_work_and_wait(pool);
			break;
		case TASK_POOL_BACKGROUND:
		case TASK_POOL_BACKGROUND_SERIAL:
			background_task_pool_work_and_wait(pool);
			break;
	}
}

void LIB_task_pool_cancel(TaskPool *pool) {
	switch (pool->type) {
		case TASK_POOL_TBB:
		case TASK_POOL_TBB_SUSPENDED:
		case TASK_POOL_NO_THREADS:
			tbb_task_pool_cancel(pool);
			break;
		case TASK_POOL_BACKGROUND:
		case TASK_POOL_BACKGROUND_SERIAL:
			background_task_pool_cancel(pool);
			break;
	}
}

bool LIB_task_pool_current_canceled(TaskPool *pool) {
	switch (pool->type) {
		case TASK_POOL_TBB:
		case TASK_POOL_TBB_SUSPENDED:
		case TASK_POOL_NO_THREADS:
			return tbb_task_pool_canceled(pool);
		case TASK_POOL_BACKGROUND:
		case TASK_POOL_BACKGROUND_SERIAL:
			return background_task_pool_canceled(pool);
	}
	ROSE_assert_msg(0, "roselib/roselib_task_pool_canceled: Control flow should not come here!");
	return false;
}

void *LIB_task_pool_user_data(TaskPool *pool) {
	return pool->userdata;
}

ThreadMutex *LIB_task_pool_user_mutex(TaskPool *pool) {
	return &pool->user_mutex;
}
