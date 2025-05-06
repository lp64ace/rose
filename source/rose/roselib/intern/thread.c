#include "MEM_guardedalloc.h"

#include "LIB_thread.h"
#include "LIB_gsqueue.h"
#include "LIB_listbase.h"
#include "LIB_utildefines.h"

#ifdef WIN32
#	include <sys/timeb.h>
#	include <windows.h>
#elif defined(__APPLE__)
#	include <sys/sysctl.h>
#	include <sys/types.h>
#else
#	include <sys/time.h>
#	include <unistd.h>
#endif

#include <time.h>
#include <errno.h>

#include "atomic_ops.h"

/* -------------------------------------------------------------------- */
/** \name Thread Pool
 * \{ */

static pthread_t mainid;
static uint thread_levels = 0; /* threads can be invoked inside threads */
static int threads_override_num = 0;

/* just a max for security reasons */
#define RE_MAX_THREAD ROSE_MAX_THREADS

typedef struct ThreadSlot {
	struct ThreadSlot *prev, *next;
	void *(*do_thread)(void *);
	void *callerdata;
	pthread_t pthread;
	int avail;
} ThreadSlot;

void LIB_threadapi_init(void) {
	mainid = pthread_self();
}
void LIB_threadapi_exit(void) {
}

void LIB_threadpool_init(struct ListBase *threadbase, void *(*do_thread)(void *), int tot) {
	int a;

	if (threadbase != NULL && tot > 0) {
		LIB_listbase_clear(threadbase);

		if (tot > RE_MAX_THREAD) {
			tot = RE_MAX_THREAD;
		}
		else if (tot < 1) {
			tot = 1;
		}

		for (a = 0; a < tot; a++) {
			ThreadSlot *tslot = MEM_callocN(sizeof(ThreadSlot), "ThreadSlot");
			LIB_addtail(threadbase, tslot);
			tslot->do_thread = do_thread;
			tslot->avail = 1;
		}
	}

	atomic_fetch_and_add_u(&thread_levels, 1);
}

int LIB_available_threads(struct ListBase *threadbase) {
	int counter = 0;

	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (tslot->avail) {
			counter++;
		}
	}

	return counter;
}

int LIB_threadpool_available_thread_index(struct ListBase *threadbase) {
	int counter = 0;

	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (tslot->avail) {
			return counter;
		}
		++counter;
	}

	return 0;
}

static void *tslot_thread_start(void *tslot_p) {
	ThreadSlot *tslot = (ThreadSlot *)tslot_p;
	return tslot->do_thread(tslot->callerdata);
}

void LIB_threadpool_insert(ListBase *threadbase, void *callerdata) {
	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (tslot->avail) {
			tslot->avail = 0;
			tslot->callerdata = callerdata;
			pthread_create(&tslot->pthread, NULL, tslot_thread_start, tslot);
			return;
		}
	}
}

void LIB_threadpool_remove(ListBase *threadbase, void *callerdata) {
	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (tslot->callerdata == callerdata) {
			pthread_join(tslot->pthread, NULL);
			tslot->callerdata = NULL;
			tslot->avail = 1;
		}
	}
}

void LIB_threadpool_remove_index(ListBase *threadbase, int index) {
	int counter = 0;

	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (counter == index && tslot->avail == 0) {
			pthread_join(tslot->pthread, NULL);
			tslot->callerdata = NULL;
			tslot->avail = 1;
			break;
		}
		++counter;
	}
}

void LIB_threadpool_clear(ListBase *threadbase) {
	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (tslot->avail == 0) {
			pthread_join(tslot->pthread, NULL);
			tslot->callerdata = NULL;
			tslot->avail = 1;
		}
	}
}

void LIB_threadpool_end(ListBase *threadbase) {
	/* Only needed if there's actually some stuff to end
	 * this way we don't end up decrementing thread_levels on an empty `threadbase`. */
	if (threadbase == NULL || LIB_listbase_is_empty(threadbase)) {
		return;
	}

	LISTBASE_FOREACH(ThreadSlot *, tslot, threadbase) {
		if (tslot->avail == 0) {
			pthread_join(tslot->pthread, NULL);
		}
	}
	LIB_freelistN(threadbase);
}

int LIB_thread_is_main(void) {
	return pthread_equal(pthread_self(), mainid);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mutex Lock
 * \{ */

void LIB_mutex_init(ThreadMutex *mutex) {
	pthread_mutex_init(mutex, NULL);
}
void LIB_mutex_end(ThreadMutex *mutex) {
	pthread_mutex_destroy(mutex);
}

ThreadMutex *LIB_mutex_alloc(void) {
	ThreadMutex *mutex = MEM_callocN(sizeof(ThreadMutex), "ThreadMutex");
	LIB_mutex_init(mutex);
	return mutex;
}
void LIB_mutex_free(ThreadMutex *mutex) {
	LIB_mutex_end(mutex);
	MEM_freeN(mutex);
}

void LIB_mutex_lock(ThreadMutex *mutex) {
	pthread_mutex_lock(mutex);
}
bool LIB_mutex_trylock(ThreadMutex *mutex) {
	return pthread_mutex_trylock(mutex) == 0;
}
void LIB_mutex_unlock(ThreadMutex *mutex) {
	pthread_mutex_unlock(mutex);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Spin Lock
 * \{ */

void LIB_spin_init(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_init(spin);
#elif defined(_MSC_VER)
	*spin = 0;
#else
	pthread_spin_init(spin, 0);
#endif
}

void LIB_spin_lock(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_lock(spin);
#elif defined(_MSC_VER)
#	if defined(_M_ARM64)
	// InterlockedExchangeAcquire takes a long arg on MSVC ARM64
	ROSE_STATIC_ASSERT(sizeof(long) == sizeof(SpinLock));
	while (InterlockedExchangeAcquire((volatile long *)spin, 1)) {
#	else
	while (InterlockedExchangeAcquire(spin, 1)) {
#	endif
		while (*spin) {
			/* Spin-lock hint for processors with hyper-threading. */
			YieldProcessor();
		}
	}
#else
	pthread_spin_lock(spin);
#endif
}

void LIB_spin_unlock(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_unlock(spin);
#elif defined(_MSC_VER)
	_ReadWriteBarrier();
	*spin = 0;
#else
	pthread_spin_unlock(spin);
#endif
}

void LIB_spin_end(SpinLock *spin) {
#if defined(__APPLE__)
	LIB_mutex_end(spin);
#elif defined(_MSC_VER)
	/* Nothing to do, spin is a simple integer type. */
	UNUSED_VARS(spin);
#else
	pthread_spin_destroy(spin);
#endif
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Thread Queue
 * \{ */

typedef struct ThreadQueue {
	GSQueue *queue;
	pthread_mutex_t mutex;
	pthread_cond_t push_cond;
	pthread_cond_t finish_cond;
	volatile int nowait;
	volatile int canceled;
} ThreadQueue;

ThreadQueue *LIB_thread_queue_init() {
    ThreadQueue *queue;

    queue = MEM_callocN(sizeof(ThreadQueue), "ThreadQueue");
    queue->queue = LIB_gsqueue_new(sizeof(void *));

    pthread_mutex_init(&queue->mutex, NULL);
	pthread_cond_init(&queue->push_cond, NULL);
	pthread_cond_init(&queue->finish_cond, NULL);

    return queue;
}

void LIB_thread_queue_free(ThreadQueue *queue) {
    /* destroy everything, assumes no one is using queue anymore */
    pthread_cond_destroy(&queue->finish_cond);
    pthread_cond_destroy(&queue->push_cond);
    pthread_mutex_destroy(&queue->mutex);

    LIB_gsqueue_free(queue->queue);

    MEM_freeN(queue);
}

void LIB_thread_queue_push(ThreadQueue *queue, void *work) {
    pthread_mutex_lock(&queue->mutex);

    LIB_gsqueue_push(queue->queue, &work);

    /* signal threads waiting to pop */
    pthread_cond_signal(&queue->push_cond);
    pthread_mutex_unlock(&queue->mutex);
}

void *LIB_thread_queue_pop(ThreadQueue *queue) {
    void *work = NULL;

    /* wait until there is work */
    pthread_mutex_lock(&queue->mutex);
    while (LIB_gsqueue_is_empty(queue->queue) && !queue->nowait) {
        pthread_cond_wait(&queue->push_cond, &queue->mutex);
    }

    /* if we have something, pop it */
    if (!LIB_gsqueue_is_empty(queue->queue)) {
        LIB_gsqueue_pop(queue->queue, &work);

        if (LIB_gsqueue_is_empty(queue->queue)) {
            pthread_cond_broadcast(&queue->finish_cond);
        }
    }

    pthread_mutex_unlock(&queue->mutex);

    return work;
}

static void wait_timeout(struct timespec *timeout, int ms) {
    ldiv_t div_result;
    long sec, usec, x;

#ifdef WIN32
    {
        struct _timeb now;
        _ftime(&now);
        sec = now.time;
        usec = now.millitm * 1000; /* microsecond precision would be better */
    }
#else
    {
        struct timeval now;
        gettimeofday(&now, NULL);
        sec = now.tv_sec;
        usec = now.tv_usec;
    }
#endif

    /* add current time + millisecond offset */
    div_result = ldiv(ms, 1000);
    timeout->tv_sec = sec + div_result.quot;

    x = usec + (div_result.rem * 1000);

    if (x >= 1000000) {
        timeout->tv_sec++;
        x -= 1000000;
    }

    timeout->tv_nsec = x * 1000;
}

void *LIB_thread_queue_pop_timeout(ThreadQueue *queue, int ms) {
    void *work = NULL;
    struct timespec timeout;

    /* double t = LIB_check_seconds_timer(); */
    wait_timeout(&timeout, ms);

    /* wait until there is work */
    pthread_mutex_lock(&queue->mutex);
    while (LIB_gsqueue_is_empty(queue->queue) && !queue->nowait) {
        if (pthread_cond_timedwait(&queue->push_cond, &queue->mutex, &timeout) == ETIMEDOUT) {
            break;
        }
		/*
		if (LIB_check_seconds_timer() - t >= ms * 0.001) {
            break;
        }
        */
		break;
    }

    /* if we have something, pop it */
    if (!LIB_gsqueue_is_empty(queue->queue)) {
        LIB_gsqueue_pop(queue->queue, &work);

        if (LIB_gsqueue_is_empty(queue->queue)) {
            pthread_cond_broadcast(&queue->finish_cond);
        }
    }

    pthread_mutex_unlock(&queue->mutex);

    return work;
}

int LIB_thread_queue_len(ThreadQueue *queue) {
    int size;

    pthread_mutex_lock(&queue->mutex);
    size = LIB_gsqueue_len(queue->queue);
    pthread_mutex_unlock(&queue->mutex);

    return size;
}

bool LIB_thread_queue_is_empty(ThreadQueue *queue) {
    bool is_empty;

    pthread_mutex_lock(&queue->mutex);
    is_empty = LIB_gsqueue_is_empty(queue->queue);
    pthread_mutex_unlock(&queue->mutex);

    return is_empty;
}

void LIB_thread_queue_nowait(ThreadQueue *queue) {
    pthread_mutex_lock(&queue->mutex);

    queue->nowait = 1;

    /* signal threads waiting to pop */
    pthread_cond_broadcast(&queue->push_cond);
    pthread_mutex_unlock(&queue->mutex);
}

void LIB_thread_queue_wait_finish(ThreadQueue *queue) {
    /* wait for finish condition */
    pthread_mutex_lock(&queue->mutex);

    while (!LIB_gsqueue_is_empty(queue->queue)) {
        pthread_cond_wait(&queue->finish_cond, &queue->mutex);
    }

    pthread_mutex_unlock(&queue->mutex);
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name System Information
 * \{ */

size_t LIB_system_thread_count() {
#ifdef WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return ROSE_MIN(1, info.dwNumberOfProcessors);
#else
	return ROSE_MIN(1, sysconf(_SC_NPROCESSORS_ONLN));
#endif
}

/** \} */
