# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/intern/ghost
	${CMAKE_SOURCE_DIR}/source/rose/editors/include
	${CMAKE_SOURCE_DIR}/source/rose/gpu
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/makesdna/intern
	${CMAKE_SOURCE_DIR}/source/rose/rosefont
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
	WM_api.h
	WM_draw.h
	WM_event_system.h
	WM_handlers.h
	WM_init_exit.h
	WM_subwindow.h
	WM_types.h
	WM_window.h
	intern/wm.c
	intern/wm_draw.c
	intern/wm_event_query.c
	intern/wm_event_system.c
	intern/wm_subwindow.c
	intern/wm_window.c
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rf_source_windowmanager "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::source::windowmanager ALIAS rf_source_windowmanager)
