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

set(SRC
	cube.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	rose::source::rosemesh
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

add_executable(cube "${SRC}")

rose_target_link_libraries(cube "${LIB}")
rose_target_include_dirs(cube "${INC}")
