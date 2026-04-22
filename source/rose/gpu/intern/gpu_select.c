#include "MEM_guardedalloc.h"

#include "GPU_select.h"

#include "LIB_rect.h"
#include "LIB_utildefines.h"

#include "gpu_select_private.h"

/* -------------------------------------------------------------------- */
/** \name Internal Types
 * \{ */

typedef enum SelectAlgo {
	/** glBegin/EndQuery(GL_SAMPLES_PASSED... ), `gpu_select_query.c`
	 * Only sets 4th component (ID) correctly. */
	ALGO_GL_QUERY = 1,
	/** Read depth buffer for every drawing pass and extract depths, `gpu_select_pick.c`
	 * Only sets 4th component (ID) correctly. */
	ALGO_GL_PICK = 2,
} SelectAlgo;

typedef struct GPUSelectState {
	/* To ignore selection id calls when not initialized */
	bool select_is_active;
	/* mode of operation */
	SelectMode mode;
	/* internal algorithm for selection */
	SelectAlgo algorithm;
	/* allow GPU_select_begin/end without drawing */
	bool use_cache;
	/**
	 * Signifies that #GPU_select_cache_begin has been called,
	 * future calls to #GPU_select_begin should initialize the cache.
	 *
	 * \note #GPU_select_cache_begin could perform initialization but doesn't as it's inconvenient
	 * for callers making the cache begin/end calls outside lower level selection logic
	 * where the `mode` to pass to #GPU_select_begin yet isn't known.
	 */
	bool use_cache_needs_init;
} GPUSelectState;

static GPUSelectState GSelectState;

/** \} */

/* -------------------------------------------------------------------- */
/** \name Public API
 * \{ */

void GPU_select_begin(GPUSelectResult *buffer, const unsigned int buffer_len, const rcti *input, SelectMode mode, int oldhits) {
	if (mode == GPU_SELECT_NEAREST_SECOND_PASS) {
		/* In the case hits was '-1',
		 * don't start the second pass since it's not going to give useful results.
		 * As well as buffer overflow in 'gpu_select_query_load_id'. */
		ROSE_assert(oldhits != -1);
	}

	GSelectState.select_is_active = true;
	GSelectState.mode = mode;

	if (ELEM(GSelectState.mode, GPU_SELECT_PICK_ALL, GPU_SELECT_PICK_NEAREST)) {
		GSelectState.algorithm = ALGO_GL_PICK;
	}
	else {
		GSelectState.algorithm = ALGO_GL_QUERY;
	}

	/* This function is called when cache has already been initialized,
	 * so only manipulate cache values when cache is pending. */
	if (GSelectState.use_cache_needs_init) {
		GSelectState.use_cache_needs_init = false;

		switch (GSelectState.algorithm) {
			case ALGO_GL_QUERY: {
				GSelectState.use_cache = false;
				break;
			}
			default: {
				GSelectState.use_cache = true;
				gpu_select_pick_cache_begin();
				break;
			}
		}
	}

	switch (GSelectState.algorithm) {
		case ALGO_GL_QUERY: {
			gpu_select_query_begin(buffer, buffer_len, input, mode, oldhits);
			break;
		}
		/* ALGO_GL_PICK */
		default: {
			gpu_select_pick_begin(buffer, buffer_len, input, mode);
			break;
		}
	}
}

bool GPU_select_load_id(unsigned int id) {
	/* if no selection mode active, ignore */
	if (!GSelectState.select_is_active) {
		return true;
	}

	switch (GSelectState.algorithm) {
		case ALGO_GL_QUERY: {
			return gpu_select_query_load_id(id);
		}
		/* ALGO_GL_PICK */
		default: {
			return gpu_select_pick_load_id(id, false);
		}
	}
}

unsigned int GPU_select_end(void) {
	unsigned int hits = 0;

	switch (GSelectState.algorithm) {
		case ALGO_GL_QUERY: {
			hits = gpu_select_query_end();
			break;
		}
		/* ALGO_GL_PICK */
		default: {
			hits = gpu_select_pick_end();
			break;
		}
	}

	GSelectState.select_is_active = false;

	return hits;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Caching
 *
 * Support multiple begin/end's as long as they are within the initial region.
 * Currently only used by #ALGO_GL_PICK.
 * \{ */

void GPU_select_cache_begin(void) {
	ROSE_assert(GSelectState.select_is_active == false);
	/* Ensure #GPU_select_cache_end is always called. */
	ROSE_assert(GSelectState.use_cache_needs_init == false);

	/* Signal that cache should be used, instead of calling the algorithms cache-begin function.
	 * This is more convenient as the exact method of selection may not be known by the caller. */
	GSelectState.use_cache_needs_init = true;
}

void GPU_select_cache_load_id(void) {
	ROSE_assert(GSelectState.use_cache == true);
	if (GSelectState.algorithm == ALGO_GL_PICK) {
		gpu_select_pick_cache_load_id();
	}
}

void GPU_select_cache_end(void) {
	if (GSelectState.algorithm == ALGO_GL_PICK) {
		ROSE_assert(GSelectState.use_cache == true);
		gpu_select_pick_cache_end();
	}
	GSelectState.use_cache = false;
	/* Paranoid assignment, should already be false. */
	GSelectState.use_cache_needs_init = false;
}

bool GPU_select_is_cached(void) {
	return GSelectState.use_cache && gpu_select_pick_is_cached();
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Utilities
 * \{ */

const GPUSelectResult *GPU_select_buffer_near(const GPUSelectResult *buffer, int hits) {
	const GPUSelectResult *buffer_near = NULL;
	unsigned int depth_min = (unsigned int)-1;
	for (int i = 0; i < hits; i++) {
		if (buffer->depth < depth_min) {
			ROSE_assert(buffer->id != -1);
			depth_min = buffer->depth;
			buffer_near = buffer;
		}
		buffer++;
	}
	return buffer_near;
}

unsigned int GPU_select_buffer_remove_by_id(GPUSelectResult *buffer, int hits, unsigned int select_id) {
	GPUSelectResult *buffer_src = buffer;
	GPUSelectResult *buffer_dst = buffer;
	int hits_final = 0;
	for (int i = 0; i < hits; i++) {
		if (buffer_src->id != select_id) {
			if (buffer_dst != buffer_src) {
				memcpy(buffer_dst, buffer_src, sizeof(GPUSelectResult));
			}
			buffer_dst++;
			hits_final += 1;
		}
		buffer_src++;
	}
	return hits_final;
}

void GPU_select_buffer_stride_realign(const rcti *src, const rcti *dst, unsigned int *r_buf) {
	const int x = dst->xmin - src->xmin;
	const int y = dst->ymin - src->ymin;

	ROSE_assert(src->xmin <= dst->xmin && src->ymin <= dst->ymin && src->xmax >= dst->xmax && src->ymax >= dst->ymax);
	ROSE_assert(x >= 0 && y >= 0);

	const int src_x = LIB_rcti_size_x(src);
	const int src_y = LIB_rcti_size_y(src);
	const int dst_x = LIB_rcti_size_x(dst);
	const int dst_y = LIB_rcti_size_y(dst);

	int last_px_id = src_x * (y + dst_y - 1) + (x + dst_x - 1);
	memset(&r_buf[last_px_id + 1], 0, (src_x * src_y - (last_px_id + 1)) * sizeof(*r_buf));

	if (last_px_id < 0) {
		/* Nothing to write. */
		ROSE_assert(last_px_id == -1);
		return;
	}

	int last_px_written = dst_x * dst_y - 1;
	const int skip = src_x - dst_x;

	while (true) {
		for (int i = dst_x; i--;) {
			r_buf[last_px_id--] = r_buf[last_px_written--];
		}
		if (last_px_written < 0) {
			break;
		}
		last_px_id -= skip;
		memset(&r_buf[last_px_id + 1], 0, skip * sizeof(*r_buf));
	}
	memset(r_buf, 0, (last_px_id + 1) * sizeof(*r_buf));
}

/** \} */
