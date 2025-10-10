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
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(ALICE
	# And yes... you are right... this is inspired through Alice in Madness
	engines/alice/alice_engine.h
	engines/alice/alice_engine.c
	engines/alice/alice_private.h
	engines/alice/alice_shader.c
)

set(BASIC
	engines/basic/basic_engine.h
	engines/basic/basic_engine.c
)

set(GLSL_SRC
	engines/alice/shaders/alice_vert.glsl

	intern/shaders/draw_shader_shared.h

)

set(SRC
	DRW_cache.h
	DRW_engine.h
	DRW_render.h

	intern/mesh/extract_mesh.h
	intern/mesh/extract_mesh_ibo_tris.cc
	intern/mesh/extract_mesh_vbo_pos.cc
	intern/mesh/extract_mesh_vbo_lnor.cc
	intern/draw_cache_private.h
	intern/draw_cache_inline.h
	intern/draw_cache.c
	intern/draw_cache.cc
	intern/draw_cache_extract_mesh.cc
	intern/draw_defines.h
	intern/draw_engine.c
	intern/draw_engine.h
	intern/draw_manager.c
	intern/draw_manager.h
	intern/draw_manager_data.c
	intern/draw_manager_exec.c
	intern/draw_pass.h
	intern/draw_state.h

	intern/shaders/draw_shader_shared.h
	
)

list(APPEND SRC ${ALICE}) # Save myself? From death? is that it? Is that why I've come here? I'm not afraid to die. Times I've welcomed death!
list(APPEND SRC ${BASIC}) # Another day, a different dream perhaps.

# -----------------------------------------------------------------------------
# Make Shader Resource

set(GLSL_C

)

foreach(GLSL_FILE ${GLSL_SRC})
	data_to_c_simple(${GLSL_FILE} GLSL_C)
endforeach()

rose_add_lib(draw_shaders "${GLSL_C}" "" "" "")
add_library(rose::source::draw_shaders ALIAS draw_shaders)

set(GLSL_SOURCE_CONTENT "")
foreach(GLSL_FILE ${GLSL_SRC})
	get_filename_component(GLSL_FILE_NAME ${GLSL_FILE} NAME)
	string(REPLACE "." "_" GLSL_FILE_NAME_UNDERSCORES ${GLSL_FILE_NAME})
	string(APPEND GLSL_SOURCE_CONTENT "SHADER_SOURCE\(datatoc_${GLSL_FILE_NAME_UNDERSCORES}, \"${GLSL_FILE_NAME}\", \"${GLSL_FILE}\"\)\n")
endforeach()

set(GLSL_SOURCE_LIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/glsl_draw_source_list.h")
file(GENERATE OUTPUT ${GLSL_SOURCE_LIST_FILE} CONTENT "${GLSL_SOURCE_CONTENT}")
list(APPEND SRC ${GLSL_SOURCE_LIST_FILE})
list(APPEND INC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(draw_shaders PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	PUBLIC rose::source::gpu
	PUBLIC rose::source::dna
	PUBLIC rose::source::roselib
	PUBLIC rose::source::rosekernel
	PUBLIC rose::source::windowmanager
	PUBLIC rose::source::draw_shaders
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(draw "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::draw ALIAS draw)
