# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
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
	testing.h
	testing_main.cc
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	PUBLIC rf::extern::gtests
	PUBLIC rf::intern::memalloc
	PUBLIC rf::source::roselib
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(testing "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::tests::testing ALIAS testing)
