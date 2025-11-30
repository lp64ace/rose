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
	WM_event.h
	WM_handler.h
	WM_window.h
	
	intern/wm.c
	intern/wm_draw.c
	intern/wm_event.c
	intern/wm_handler.c
	intern/wm_window.c
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::native
	rose::source::roselib
	rose::source::rosekernel
	rose::source::rosemesh
	rose::source::rosefont
	rose::source::gpu
	rose::source::draw
	rose::source::io::fbx
	rose::source::editors::space_api
	rose::source::editors::space_statusbar
	rose::source::editors::space_topbar
	rose::source::editors::space_view3d
	rose::source::editors::screen

	rose::resources::models
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(windowmanager "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::windowmanager ALIAS windowmanager)
