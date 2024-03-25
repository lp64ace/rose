# -----------------------------------------------------------------------------
# Generic Variables

set(DNA_TYPES_HEADERS
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_defines.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_ID.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_ID_enums.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_listbase.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_screen_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_sdna_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_space_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_vec_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_windowmanager_types.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_workspace_types.h
)

set(DNA_DEFAULTS_HEADERS
	
)

# -----------------------------------------------------------------------------
# Add Sub-Directories

add_subdirectory(rose)
add_subdirectory(creator)
