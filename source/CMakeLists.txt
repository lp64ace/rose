# -----------------------------------------------------------------------------
# Declare DNA Files

set(DNA_TYPEDEF_HEADERS
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_customdata_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_ID.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_ID_enums.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_listbase.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_mesh_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_meshdata_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_object_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_vector_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_sdna_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_space_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_userdef_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_scene_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_screen_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_windowmanager_types.h
)

set(DNA_DEFAULT_HEADERS
)

# -----------------------------------------------------------------------------
# Add Sub-Directories

add_subdirectory(rose)
add_subdirectory(creator)
