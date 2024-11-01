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
	${GLEW_INCLUDE_DIRS}

)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	tiny_window.h
	tiny_window.hh
	
	intern/api.cc
	intern/tiny_window.cc
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

find_package(OpenGL REQUIRED)

set(LIB
	# Internal Include Directories
	rose::intern::guardedalloc
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	
	# External Include Directories
	
)

if(ROSE_BUILD_WIN32)
	list(APPEND SRC intern/tiny_window_win32.cc)
endif()
if(ROSE_BUILD_X11)
	list(APPEND INC ${X11_INCLUDE_DIR})
	list(APPEND LIB ${X11_LIBRARIES})
	list(APPEND SRC intern/tiny_window_linux.cc)
	
	if(WITH_X11_XINPUT)
		add_definitions(-DWITH_X11_XINPUT)
		
		list(APPEND INC ${X11_XINPUT_INCLUDE_PATH})
		list(APPEND LIB ${X11_XINPUT_LIB})
	endif()
endif()

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(tiny_window "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::tiny_window ALIAS tiny_window)


