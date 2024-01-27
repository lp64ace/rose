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
	
)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	glib.h
	intern/platform.h
	intern/platform.hh
	intern/platform.cc
	intern/platforms/platform_win32.hh
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Executable

if(WIN32)
	list(APPEND SRC intern/platforms/platform_win32.cc)
endif()

rose_add_lib(rf_intern_ghost "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rf::intern::ghost ALIAS rf_intern_ghost)
