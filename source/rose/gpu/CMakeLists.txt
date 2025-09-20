# -----------------------------------------------------------------------------
# Builtin Shaders

set(GLSL_SRC
	GPU_shader_shared.h
	GPU_shader_shared_utils.h
	
	shaders/common/gpu_shader_common_color_ramp.glsl
	shaders/common/gpu_shader_common_color_utils.glsl
	shaders/common/gpu_shader_common_curves.glsl
	shaders/common/gpu_shader_common_hash.glsl
	shaders/common/gpu_shader_common_math.glsl
	shaders/common/gpu_shader_common_math_utils.glsl
	shaders/common/gpu_shader_common_mix_rgb.glsl
	shaders/common/gpu_shader_math_base_lib.glsl
	shaders/common/gpu_shader_math_fast_lib.glsl
	shaders/common/gpu_shader_math_matrix_lib.glsl
	shaders/common/gpu_shader_math_rotation_lib.glsl
	shaders/common/gpu_shader_math_vector_lib.glsl
	shaders/common/gpu_shader_test_lib.glsl
	shaders/common/gpu_shader_utildefines_lib.glsl
	
	shaders/opengl/glsl_shader_defines.glsl
	
	shaders/gpu_shader_2D_image_rect_vert.glsl
	shaders/gpu_shader_2D_image_vert.glsl
	shaders/gpu_shader_2D_vert.glsl
	shaders/gpu_shader_2D_widget_base_frag.glsl
	shaders/gpu_shader_2D_widget_base_vert.glsl
	shaders/gpu_shader_2D_widget_shadow_frag.glsl
	shaders/gpu_shader_2D_widget_shadow_vert.glsl
	shaders/gpu_shader_3D_clipped_uniform_color_vert.glsl
	shaders/gpu_shader_3D_vert.glsl
	shaders/gpu_shader_cfg_world_clip_lib.glsl
	shaders/gpu_shader_colorspace_lib.glsl
	shaders/gpu_shader_image_color_frag.glsl
	shaders/gpu_shader_image_overlays_merge_frag.glsl
	shaders/gpu_shader_image_overlays_stereo_merge_frag.glsl
	shaders/gpu_shader_mandelbrot.glsl
	shaders/gpu_shader_test_frag.glsl
	shaders/gpu_shader_test_vert.glsl
	shaders/gpu_shader_text_frag.glsl
	shaders/gpu_shader_text_vert.glsl
	shaders/gpu_shader_uniform_color_frag.glsl
	
	shaders/test/gpu_compute_1d_test.glsl
	shaders/test/gpu_compute_2d_test.glsl
)

set(GLSL_C
)

foreach(GLSL_FILE ${GLSL_SRC})
	data_to_c_simple(${GLSL_FILE} GLSL_C)
endforeach()

set(SHADERS_C
)

list(APPEND SHADERS_C ${GLSL_C})

rose_add_lib(shaders "${SHADERS_C}" "" "" "")
add_library(rose::source::shaders ALIAS shaders)

set(GLSL_SOURCE_CONTENT
	""
)
foreach(GLSL_FILE ${GLSL_SRC})
	get_filename_component(GLSL_FILE_NAME ${GLSL_FILE} NAME)
	string(REPLACE "." "_" GLSL_FILE_NAME_UNDERSCORES ${GLSL_FILE_NAME})
	string(APPEND GLSL_SOURCE_CONTENT "SHADER_SOURCE\(datatoc_${GLSL_FILE_NAME_UNDERSCORES}, \"${GLSL_FILE_NAME}\", \"${GLSL_FILE}\"\)\n")
endforeach()

set(GPU_SHADER_SOURCE_FILE
	"${CMAKE_CURRENT_BINARY_DIR}/gpu_shader_source_list.h"
)
file(GENERATE OUTPUT ${GPU_SHADER_SOURCE_FILE} CONTENT "${GLSL_SOURCE_CONTENT}")

# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	${CMAKE_CURRENT_BINARY_DIR}
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories

set(INC_SYS
	# External System Include Directories
	${GLEW_INCLUDE_DIRS}
	${PTHREADS_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	GPU_batch.h
	GPU_batch_presets.h
	GPU_batch_utils.h
	GPU_common.h
	GPU_common_types.h
	GPU_compute.h
	GPU_context.h
	GPU_debug.h
	GPU_drawlist.h
	GPU_framebuffer.h
	GPU_immediate.h
	GPU_immediate_util.h
	GPU_index_buffer.h
	GPU_info.h
	GPU_init_exit.h
	GPU_material.h
	GPU_matrix.h
	GPU_platform.h
	GPU_primitive.h
	GPU_select.h
	GPU_shader.h
	GPU_shader_builtin.h
	GPU_shader_shared.h
	GPU_shader_shared_utils.h
	GPU_state.h
	GPU_storage_buffer.h
	GPU_texture.h
	GPU_uniform_buffer.h
	GPU_vertex_buffer.h
	GPU_vertex_format.h
	GPU_viewport.h

	intern/gpu_backend.hh
	intern/gpu_batch.cc
	intern/gpu_batch_presets.c
	intern/gpu_batch_private.hh
	intern/gpu_batch_utils.c
	intern/gpu_compute.cc
	intern/gpu_context.cc
	intern/gpu_context_private.hh
	intern/gpu_debug.cc
	intern/gpu_debug_private.hh
	intern/gpu_drawlist.cc
	intern/gpu_drawlist_private.hh
	intern/gpu_framebuffer.cc
	intern/gpu_framebuffer_private.hh
	intern/gpu_immediate.cc
	intern/gpu_immediate_private.hh
	intern/gpu_immediate_util.c
	intern/gpu_index_buffer.cc
	intern/gpu_index_buffer_private.hh
	intern/gpu_info.cc
	intern/gpu_info_private.hh
	intern/gpu_init_exit.c
	intern/gpu_material.c
	intern/gpu_material_library.h
	intern/gpu_matrix.cc
	intern/gpu_matrix_private.h
	intern/gpu_platform.cc
	intern/gpu_platform_private.hh
	intern/gpu_private.h
	intern/gpu_query.cc
	intern/gpu_query.hh
	intern/gpu_select.c
	intern/gpu_select_pick.c
	intern/gpu_select_private.h
	intern/gpu_select_sample_query.cc
	intern/gpu_shader.cc
	intern/gpu_shader_builtin.cc
	intern/gpu_shader_create_info.cc
	intern/gpu_shader_create_info.hh
	intern/gpu_shader_create_info_private.hh
	intern/gpu_shader_dependency.cc
	intern/gpu_shader_dependency_private.h
	intern/gpu_shader_interface.cc
	intern/gpu_shader_interface.hh
	intern/gpu_shader_log.cc
	intern/gpu_shader_private.hh
	intern/gpu_state.cc
	intern/gpu_state_private.hh
	intern/gpu_storage_buffer.cc
	intern/gpu_storage_buffer_private.hh
	intern/gpu_texture.cc
	intern/gpu_texture_private.hh
	intern/gpu_uniform_buffer.cc
	intern/gpu_uniform_buffer_private.hh
	intern/gpu_vertex_buffer.cc
	intern/gpu_vertex_buffer_private.hh
	intern/gpu_vertex_format.cc
	intern/gpu_vertex_format_private.h
	intern/gpu_viewport.cc
)

set(OPENGL_SRC
	opengl/gl_backend.cc
	opengl/gl_batch.cc
	opengl/gl_compute.cc
	opengl/gl_context.cc
	opengl/gl_debug.cc
	opengl/gl_debug_layer.cc
	opengl/gl_drawlist.cc
	opengl/gl_framebuffer.cc
	opengl/gl_immediate.cc
	opengl/gl_index_buffer.cc
	opengl/gl_query.cc
	opengl/gl_shader.cc
	opengl/gl_shader_interface.cc
	opengl/gl_shader_log.cc
	opengl/gl_state.cc
	opengl/gl_storage_buffer.cc
	opengl/gl_texture.cc
	opengl/gl_uniform_buffer.cc
	opengl/gl_vertex_array.cc
	opengl/gl_vertex_buffer.cc
	opengl/gl_backend.hh
	opengl/gl_batch.hh
	opengl/gl_common.hh
	opengl/gl_compute.hh
	opengl/gl_context.hh
	opengl/gl_debug.hh
	opengl/gl_drawlist.hh
	opengl/gl_framebuffer.hh
	opengl/gl_immediate.hh
	opengl/gl_index_buffer.hh
	opengl/gl_primitive.hh
	opengl/gl_query.hh
	opengl/gl_shader.hh
	opengl/gl_shader_interface.hh
	opengl/gl_state.hh
	opengl/gl_storage_buffer.hh
	opengl/gl_texture.hh
	opengl/gl_uniform_buffer.hh
	opengl/gl_vertex_array.hh
	opengl/gl_vertex_buffer.hh
)

list(APPEND SRC ${OPENGL_SRC})

set(INFO_SRC
	infos/gpu_clip_planes_info.hh
	infos/gpu_interface_info.hh
	infos/gpu_shader_2D_image_overlays_merge_info.hh
	infos/gpu_shader_2D_image_overlays_stereo_merge_info.hh
	infos/gpu_shader_2D_image_rect_color_info.hh
	infos/gpu_shader_2D_widget_info.hh
	infos/gpu_shader_3D_uniform_color_info.hh
	infos/gpu_shader_mandelbrot_info.hh
	infos/gpu_shader_text_info.hh
	infos/gpu_srgb_to_framebuffer_space.hh
	
	infos/gpu_shader_test_info.hh
)

set(SHADER_CREATE_INFOS_CONTENT
	""
)
foreach(DESCRIPTOR_FILE ${INFO_SRC})
	string(APPEND SHADER_CREATE_INFOS_CONTENT "#include \"${DESCRIPTOR_FILE}\"\n")
endforeach()

set(SHADER_CREATE_INFO_LIST_FILE
	"${CMAKE_CURRENT_BINARY_DIR}/gpu_shader_create_info_list.hh"
)
file(GENERATE OUTPUT ${SHADER_CREATE_INFO_LIST_FILE} CONTENT "${SHADER_CREATE_INFOS_CONTENT}")

list(APPEND SRC ${GPU_SHADER_SOURCE_FILE})
list(APPEND SRC ${SHADER_CREATE_INFO_LIST_FILE})

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::oswin
	rose::source::draw
	rose::source::roselib
	rose::source::rosefont
	rose::source::shaders
	
	# External Library Dependencies
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(gpu "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::gpu ALIAS gpu)

# -----------------------------------------------------------------------------
# Define Include Directories (Test)

set(INC
	# Internal Include Directories
	PUBLIC .
	${CMAKE_CURRENT_BINARY_DIR}
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories (Test)

set(INC_SYS
	# External System Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files (Test)

set(TEST
	test/gpu_test.cc
	test/gpu_test.hh
	test/gpu_test_compute.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::oswin
	rose::source::draw
	rose::source::roselib
	rose::source::gpu
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Test

if(BUILD_GRAPHIC_TESTS)
	rose_add_test_executable(gpu "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")
endif()
