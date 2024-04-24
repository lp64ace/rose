# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/rosekernel
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories
	${PTHREADS_INC}
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	RLO_readfile.h
	RLO_read_write.h
	RLO_writefile.h
	RLO_rose_defs.h
	intern/writefile.cc
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::source::dna
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_roseloader "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::roseloader ALIAS rf_source_roseloader)
