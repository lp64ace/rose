# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	.
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/editors/include
	${CMAKE_SOURCE_DIR}/source/rose/gpu
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	${CMAKE_SOURCE_DIR}/source/rose/rosekernel
	${CMAKE_SOURCE_DIR}/source/rose/windowmanager
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	interface.c
	interface_draw.c
	interface_intern.h
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Library Dependencies
	rf::source::editors::util
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_editors_interface "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::editors::interface ALIAS rf_source_editors_interface)
