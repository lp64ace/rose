# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	.
	
	# Internal Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	wcwidth.c
	wcwidth.h
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_extern_wcwidth "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::extern::wcwidth ALIAS rf_extern_wcwidth)
