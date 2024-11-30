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
	RLO_read_write.h
	RLO_readfile.h
	RLO_writefile.h
	
	intern/readfile.cc
	intern/versioning_userdef.cc
	intern/writefile.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::dna
	rose::source::roselib
	rose::source::rosekernel
	rose::source::translator
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(roseloader "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::roseloader ALIAS roseloader)
