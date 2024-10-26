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
	glib.h
	
	intern/api.cc
	intern/display.hh
	intern/platform.cc
	intern/platform.hh
	intern/window.cc
	intern/window.hh
)

set(SRC_WIN32
	intern/win32/win32_display.cc
	intern/win32/win32_display.hh
	intern/win32/win32_platform.cc
	intern/win32/win32_platform.hh
	intern/win32/win32_window.cc
	intern/win32/win32_window.hh
)

set(SRC_X11
	intern/x11/x11_display.cc
	intern/x11/x11_display.hh
	intern/x11/x11_platform.cc
	intern/x11/x11_platform.hh
	intern/x11/x11_window.cc
	intern/x11/x11_window.hh
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

if(ROSE_BUILD_X11)
	list(APPEND INC ${X11_INCLUDE_DIR})
	list(APPEND SRC ${SRC_X11})
	list(APPEND LIB ${X11_LIBRARIES})
endif()

if(ROSE_BUILD_WIN32)
	list(APPEND SRC ${SRC_WIN32})
endif()

rose_add_lib(ghost "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::ghost ALIAS ghost)

target_compile_definitions(ghost
    PRIVATE HAS_X11=$<BOOL:${ROSE_BUILD_X11}>
    PRIVATE HAS_WIN32=$<BOOL:${ROSE_BUILD_WIN32}>
)
