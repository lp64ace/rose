# -----------------------------------------------------------------------------
# Generic Variables

set(DNA_TYPES_HEADERS
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_ID.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_ID_enums.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_listbase.h
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/DNA_windowmanager.h
)

set(DNA_DEFAULTS_HEADERS
	
)

# -----------------------------------------------------------------------------
# Add Sub-Directories

add_subdirectory(rose)
add_subdirectory(creator)
