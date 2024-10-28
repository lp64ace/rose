#pragma once

#include "LIB_set.hh"
#include "LIB_vector.hh"

#include "intern/gpu_context_private.hh"

#include "gl_batch.hh"
#include "gl_state.hh"

#include <mutex>

#include <GL/glew.h>

namespace rose {
namespace gpu {

class GLSharedOrphanLists {
public:
	/** Mutex for below structures. */
	std::mutex lists_mutex;
	/** Buffers and textures are shared across context. Any context can free them. */
	Vector<GLuint> textures;
	Vector<GLuint> buffers;

public:
	void orphans_clear();
};

class GLContext : public Context {
public:
	/** Capabilities. */

	static GLint max_cubemap_size;
	static GLint max_ubo_size;
	static GLint max_ubo_binds;
	static GLint max_ssbo_size;
	static GLint max_ssbo_binds;

	/** Extensions. */

	static bool base_instance_support;
	static bool clear_texture_support;
	static bool copy_image_support;
	static bool debug_layer_support;
	static bool direct_state_access_support;
	static bool explicit_location_support;
	static bool geometry_shader_invocations;
	static bool fixed_restart_index_support;
	static bool framebuffer_fetch_support;
	static bool layered_rendering_support;
	static bool native_barycentric_support;
	static bool multi_bind_support;
	static bool multi_bind_image_support;
	static bool multi_draw_indirect_support;
	static bool shader_draw_parameters_support;
	static bool stencil_texturing_support;
	static bool texture_barrier_support;
	static bool texture_cube_map_array_support;
	static bool texture_filter_anisotropic_support;
	static bool texture_gather_support;
	static bool texture_storage_support;
	static bool vertex_attrib_binding_support;

	/** Workarounds. */

	static bool debug_layer_workaround;
	static bool unused_fb_slot_workaround;
	static bool generate_mipmap_workaround;
	static float derivative_signs[2];

	/** VBO for missing vertex attrib binding. Avoid undefined behavior on some implementation. */
	GLuint default_attr_vbo_;

	/** Used for debugging purpose. Bitflags of all bound slots. */
	uint16_t bound_ubo_slots;

private:
	/**
	 * #GPUBatch & #GPUFramebuffer have references to the context they are from, in the case the
	 * context is destroyed, we need to remove any reference to it.
	 */
	Set<GLVaoCache *> vao_caches_;
	/** Mutex for below structures. */
	std::mutex lists_mutex_;
	/** VertexArrays and FrameBuffers are not shared across context. */
	Vector<GLuint> orphaned_vertarrays_;
	Vector<GLuint> orphaned_framebuffers_;
	/** Data owned by #GLBackend, do not delete. */
	GLSharedOrphanLists &shared_orphan_list_;

public:
	GLContext(void *window, GLSharedOrphanLists &shared_orphan_list);
	~GLContext();

	void activate() override;
	void deactivate() override;

	void flush() override;
	void finish() override;

	static GLContext *get() {
		return static_cast<GLContext *>(Context::get());
	}
	static GLStateManager *state_manager_active_get() {
		GLContext *ctx = GLContext::get();
		return static_cast<GLStateManager *>(ctx->state_manager);
	};

	void vao_free(GLuint vao_id);
	void fbo_free(GLuint vao_id);

	static void buf_free(GLuint buf_id);
	static void tex_free(GLuint tex_id);

	void vao_cache_register(GLVaoCache *cache);
	void vao_cache_unregister(GLVaoCache *cache);

private:
	static void orphans_add(Vector<GLuint> &orphan_list, std::mutex &list_mutex, GLuint id);
	void orphans_clear();
};

}  // namespace gpu
}  // namespace rose
