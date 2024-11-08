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
	screen_intern.h
	
	area.c
	region.c
	screen.c
	screen_edit.c
	screen_geometry.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::rosekernel
	rose::source::dna
	rose::source::gpu
	rose::source::windowmanager
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(screen "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::screen ALIAS screen)
