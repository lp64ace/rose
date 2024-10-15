# -----------------------------------------------------------------------------
# Declare DNA Files

set(DNA_TYPEDEF_HEADERS
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_ID.h
	${CMAKE_SOURCE_DIR}/source/rose/makedna/DNA_listbase.h
)

set(DNA_DEFAULT_HEADERS
)

# -----------------------------------------------------------------------------
# Add Sub-Directories

add_subdirectory(rose)
