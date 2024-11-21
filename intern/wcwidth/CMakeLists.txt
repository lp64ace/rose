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
	wcwidth.h
	wcwidth.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(wcwidth "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::wcwidth ALIAS wcwidth)
