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
	GTK_api.h
	GTK_backend_type.h
	GTK_window_type.h
	
	intern/gtk_api.cc
	intern/gtk_render.cc
	intern/gtk_render.hh
	intern/gtk_window.cc
	intern/gtk_window.hh
	intern/gtk_window_manager.cc
	intern/gtk_window_manager.hh
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
	list(APPEND LIB dwmapi.lib)
	list(APPEND SRC
		intern/win32/gtk_win32_render.cc
		intern/win32/gtk_win32_render.hh
		intern/win32/gtk_win32_window.cc
		intern/win32/gtk_win32_window.hh
		intern/win32/gtk_win32_window_manager.cc
		intern/win32/gtk_win32_window_manager.hh
	)
endif()
if(ROSE_BUILD_X11)
	list(APPEND INC ${X11_INCLUDE_DIR})
	list(APPEND LIB ${X11_LIBRARIES})
	list(APPEND SRC
		intern/x11/gtk_x11_render.cc
		intern/x11/gtk_x11_render.hh
		intern/x11/gtk_x11_window.cc
		intern/x11/gtk_x11_window.hh
		intern/x11/gtk_x11_window_manager.cc
		intern/x11/gtk_x11_window_manager.hh
	)
	
	if(WITH_X11_XINPUT)
		add_definitions(-DWITH_X11_XINPUT)
		
		list(APPEND INC ${X11_XINPUT_INCLUDE_PATH})
		list(APPEND LIB ${X11_XINPUT_LIB})
	endif()
endif()

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(native "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::intern::native ALIAS native)
