#pragma once

#include "LIB_vector.hh"

#include "intern/gpu_query.hh"

#include <GL/glew.h>

namespace rose::gpu {

class GLQueryPool : public QueryPool {
private:
	/** Contains queries object handles. */
	Vector<GLuint, QUERY_MIN_LEN> query_ids_;
	/** Type of this query pool. */
	QueryType type_;
	/** Associated GL type. */
	GLenum gl_type_;
	/** Number of queries that have been issued since last initialization.
	 * Should be equal to query_ids_.size(). */
	unsigned int query_issued_;
	/** Can only be initialized once. */
	bool initialized_ = false;

public:
	~GLQueryPool();

	void init(QueryType type) override;

	void begin_query() override;
	void end_query() override;

	void get_occlusion_result(MutableSpan<uint32_t> r_values) override;
};

static inline GLenum to_gl(QueryType type) {
	if (type == GPU_QUERY_OCCLUSION) {
		/* TODO(fclem): try with GL_ANY_SAMPLES_PASSED​. */
		return GL_SAMPLES_PASSED;
	}
	ROSE_assert(0);
	return GL_SAMPLES_PASSED;
}

}  // namespace rose::gpu
