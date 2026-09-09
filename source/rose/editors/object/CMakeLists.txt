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
	object_select.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(object "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::object ALIAS object)
