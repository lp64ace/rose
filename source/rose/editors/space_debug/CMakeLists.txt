# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC ../include
	
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
	space_debug.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::rosekernel
	rose::source::roseloader
	rose::source::dna
	rose::source::windowmanager
	rose::source::editors::screen
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(space_debug "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::space_debug ALIAS space_debug)
