#include "MEM_guardedalloc.h"

#include "GPU_framebuffer.h"
#include "GPU_select.h"
#include "GPU_state.h"

#include "LIB_rect.h"
#include "LIB_bitmap.h"
#include "LIB_utildefines.h"
#include "LIB_vector.hh"

#include <cstdlib>

#include "gpu_backend.hh"
#include "gpu_select_private.h"
#include "gpu_query.hh"

using namespace rose;
using namespace rose::gpu;

typedef struct GPUSelectQueryState {
	/** Tracks whether a query has been issued so that gpu_load_id can end the previous one. */
	bool query_issued;
	/** GPU queries abstraction. Contains an array of queries. */
	QueryPool *queries;
	/** Array holding the id corresponding id to each query. */
	Vector<unsigned int, QUERY_MIN_LEN> *ids;
	/** Cache on initialization. */
	GPUSelectResult *buffer;
	/** The capacity of the `buffer` array. */
	unsigned int buffer_len;
	/** Mode of operation. */
	SelectMode mode;
	unsigned int index;
	int oldhits;

	/** Previous state to restore after drawing. */
	int viewport[4];
	int scissor[4];
	WriteMask write_mask;
	DepthTest depth_test;
} GPUSelectQueryState;

static GPUSelectQueryState GQueryState;

void gpu_select_query_begin(GPUSelectResult *buffer, unsigned int buffer_len, const rcti *input, const SelectMode mode, int oldhits) {
	GQueryState.query_issued = false;
	GQueryState.buffer = buffer;
	GQueryState.buffer_len = buffer_len;
	GQueryState.mode = mode;
	GQueryState.index = 0;
	GQueryState.oldhits = oldhits;

	GQueryState.ids = new Vector<unsigned int, QUERY_MIN_LEN>();
	GQueryState.queries = GPUBackend::get()->querypool_alloc();
	GQueryState.queries->init(GPU_QUERY_OCCLUSION);

	GQueryState.write_mask = GPU_write_mask_get();
	GQueryState.depth_test = GPU_depth_test_get();
	GPU_scissor_get(GQueryState.scissor);
	GPU_viewport_size_get_i(GQueryState.viewport);

	/* Write to color buffer. Seems to fix issues with selecting alpha blended geom (see T7997). */
	GPU_color_mask(true, true, true, true);

	/* In order to save some fill rate we minimize the viewport using rect.
	 * We need to get the region of the viewport so that our geometry doesn't
	 * get rejected before the depth test. Should probably cull rect against
	 * the viewport but this is a rare case I think */

	int viewport[4] = {UNPACK2(GQueryState.viewport), LIB_rcti_size_x(input), LIB_rcti_size_y(input)};

	GPU_viewport(UNPACK4(viewport));
	GPU_scissor(UNPACK4(viewport));
	GPU_scissor_test(false);

	/* occlusion queries operates on fragments that pass tests and since we are interested on all
	 * objects in the view frustum independently of their order, we need to disable the depth test */
	if (mode == GPU_SELECT_ALL) {
		/* #glQueries on Windows+Intel drivers only works with depth testing turned on.
		 * See T62947 for details */
		GPU_depth_test(GPU_DEPTH_ALWAYS);
		GPU_depth_mask(true);
	}
	else if (mode == GPU_SELECT_NEAREST_FIRST_PASS) {
		GPU_depth_test(GPU_DEPTH_LESS_EQUAL);
		GPU_depth_mask(true);
		GPU_clear_depth(1.0f);
	}
	else if (mode == GPU_SELECT_NEAREST_SECOND_PASS) {
		GPU_depth_test(GPU_DEPTH_EQUAL);
		GPU_depth_mask(false);
	}
}

bool gpu_select_query_load_id(unsigned int id) {
	if (GQueryState.query_issued) {
		GQueryState.queries->end_query();
	}

	GQueryState.queries->begin_query();
	GQueryState.ids->append(id);
	GQueryState.query_issued = true;

	if (GQueryState.mode == GPU_SELECT_NEAREST_SECOND_PASS) {
		/* Second pass should never run if first pass fails,
		 * can read past `buffer_len` in this case. */
		ROSE_assert(GQueryState.oldhits != -1);
		if (GQueryState.index < GQueryState.oldhits) {
			if (GQueryState.buffer[GQueryState.index].id == id) {
				GQueryState.index++;
				return true;
			}
			return false;
		}
	}
	return true;
}

unsigned int gpu_select_query_end() {
	unsigned int hits = 0;
	const unsigned int maxhits = GQueryState.buffer_len;

	if (GQueryState.query_issued) {
		GQueryState.queries->end_query();
	}

	Span<unsigned int> ids = *GQueryState.ids;
	Vector<unsigned int, QUERY_MIN_LEN> result(ids.size());
	GQueryState.queries->get_occlusion_result(result);

	for (size_t i = 0; i < result.size(); i++) {
		if (result[i] != 0) {
			if (GQueryState.mode != GPU_SELECT_NEAREST_SECOND_PASS) {
				if (hits < maxhits) {
					GQueryState.buffer[hits].depth = 0xFFFF;
					GQueryState.buffer[hits].id = ids[i];
					hits++;
				}
				else {
					hits = -1;
					break;
				}
			}
			else {
				/* search in buffer and make selected object first */
				for (size_t j = 0; j < GQueryState.oldhits; j++) {
					if (GQueryState.buffer[j].id == ids[i]) {
						GQueryState.buffer[j].depth = 0;
					}
				}
				break;
			}
		}
	}

	MEM_delete(GQueryState.queries);
	MEM_delete(GQueryState.ids);

	GPU_write_mask(GQueryState.write_mask);
	GPU_depth_test(GQueryState.depth_test);
	GPU_viewport(UNPACK4(GQueryState.viewport));

	return hits;
}
