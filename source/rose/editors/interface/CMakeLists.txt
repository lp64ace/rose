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
	interface_intern.h
	
	interface.c
	interface_draw.c
	interface_layout.cc
	interface_widgets.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	rose::source::gpu
	rose::source::windowmanager
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(interface "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::interface ALIAS interface)
