# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	datatoc.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Executable

add_executable(datatoc ${SRC})
add_executable(rose::source::datatoc ALIAS datatoc)

rose_target_link_libraries(datatoc "${LIB}")
rose_target_include_dirs(datatoc "${INC}")
