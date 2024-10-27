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
	WM_api.h
	WM_draw.h
	WM_window.h
	
	intern/wm.c
	intern/wm_draw.c
	intern/wm_window.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::extern::tiny_window
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	rose::source::gpu
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(windowmanager "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::windowmanager ALIAS windowmanager)
