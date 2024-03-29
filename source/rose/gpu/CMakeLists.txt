if(NOT WITH_OPENGL_BACKEND AND NOT WITH_METAL_BACKEND AND NOT WITH_HEADLESS)
  add_definitions(-DWITH_OPENGL_BACKEND)
endif()

# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/rosefont
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	
	# External Include Directores
	${CMAKE_SOURCE_DIR}/extern/glew/include
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories
	
set(INC_SYS
	# External Include Directories
	${PTHREADS_INC}
	
)

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

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	rf::intern::ghost
	rf::source::roselib
	
	# External Include Directories
	${GLEW_LIBRARIES}
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Builtin Shaders

set(GLSL_SRC
	shaders/gpu_shader_2D_image_rect_vert.glsl
	shaders/gpu_shader_2D_image_vert.glsl
	shaders/gpu_shader_2D_vert.glsl
	shaders/gpu_shader_2D_widget_base_frag.glsl
	shaders/gpu_shader_2D_widget_base_vert.glsl
	shaders/gpu_shader_2D_widget_shadow_frag.glsl
	shaders/gpu_shader_2D_widget_shadow_vert.glsl
	shaders/gpu_shader_colorspace_lib.glsl
	shaders/gpu_shader_image_color_frag.glsl
	shaders/gpu_shader_image_overlays_merge_frag.glsl
	shaders/gpu_shader_image_overlays_stereo_merge_frag.glsl
	shaders/gpu_shader_text_frag.glsl
	shaders/gpu_shader_text_vert.glsl

	shaders/gpu_shader_test_frag.glsl
	shaders/gpu_shader_test_vert.glsl

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
)

set(GLSL_C
)
foreach(GLSL_FILE ${GLSL_SRC})
	data_to_c_simple(${GLSL_FILE} GLSL_C)
endforeach()

set(SHADER_C
)
list(APPEND SHADER_C ${GLSL_C})

rose_add_lib(rf_source_gpu_shaders "${SHADER_C}" "" "" "")
add_library(rf::source::gpu_shaders ALIAS rf_source_gpu_shaders)

list(APPEND LIB
	rf::source::gpu_shaders
)

set(GLSL_SOURCE_CONTENT
	""
)
foreach(GLSL_FILE ${GLSL_SRC})
  get_filename_component(GLSL_FILE_NAME ${GLSL_FILE} NAME)
  string(REPLACE "." "_" GLSL_FILE_NAME_UNDERSCORES ${GLSL_FILE_NAME})
  string(APPEND GLSL_SOURCE_CONTENT "SHADER_SOURCE\(datatoc_${GLSL_FILE_NAME_UNDERSCORES}, \"${GLSL_FILE_NAME}\", \"${GLSL_FILE}\"\)\n")
endforeach()

set(glsl_source_list_file
	"${CMAKE_CURRENT_BINARY_DIR}/glsl_gpu_source_list.h"
)
file(GENERATE OUTPUT ${glsl_source_list_file} CONTENT "${GLSL_SOURCE_CONTENT}")
list(APPEND SRC ${glsl_source_list_file})
list(APPEND INC ${CMAKE_CURRENT_BINARY_DIR})

set(SRC_SHADER_CREATE_INFOS
	infos/gpu_interface_info.hh
	infos/gpu_shader_2D_image_overlays_merge_info.hh
	infos/gpu_shader_2D_image_overlays_stereo_merge_info.hh
	infos/gpu_shader_2D_image_rect_color_info.hh
	infos/gpu_shader_2D_widget_info.hh
	infos/gpu_shader_text_info.hh
	infos/gpu_srgb_to_framebuffer_space.hh
	
	infos/gpu_shader_test_info.hh
)

set(SHADER_CREATE_INFOS_CONTENT
	""
)
foreach(DESCRIPTOR_FILE ${SRC_SHADER_CREATE_INFOS})
  string(APPEND SHADER_CREATE_INFOS_CONTENT "#include \"${DESCRIPTOR_FILE}\"\n")
endforeach()

set(shader_create_info_list_file
	"${CMAKE_CURRENT_BINARY_DIR}/gpu_shader_create_info_list.hh"
)
file(GENERATE OUTPUT ${shader_create_info_list_file} CONTENT "${SHADER_CREATE_INFOS_CONTENT}")

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_gpu "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::gpu ALIAS rf_source_gpu)
