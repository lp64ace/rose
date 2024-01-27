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
	${CMAKE_SOURCE_DIR}/extern/glew/include
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	glib.h
	glib_types.h
	intern/context.cc
	intern/context.hh
	intern/glib.cc
	intern/platform.cc
	intern/platform.h
	intern/platform.hh
	intern/window.cc
	intern/window.hh
	
	intern/platforms/platform_win32.hh
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	
	# External Include Directories
	glew
	
)

# -----------------------------------------------------------------------------
# Declare Executable

if(WIN32)
	list(APPEND SRC intern/platforms/platform_win32.cc)
	list(APPEND LIB opengl32.lib)
endif()

rose_add_lib(rf_intern_ghost "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::intern::ghost ALIAS rf_intern_ghost)
