# -----------------------------------------------------------------------------
# Builtin Shaders

set(GLSL_SRC
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

set(SHADERS_C
)

list(APPEND SHADERS_C ${GLSL_C})

rose_add_lib(shaders "${SHADERS_C}" "" "" "")
add_library(rose::source::shaders ALIAS shaders)

# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	
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
	GPU_context.h
	
	intern/gpu_context.cc
	intern/gpu_context_private.h
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::shaders
	
	# External Library Dependencies
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(gpu "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::gpu ALIAS gpu)
