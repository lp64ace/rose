# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/intern/memalloc
	${CMAKE_SOURCE_DIR}/source/rose/gpu
	${CMAKE_SOURCE_DIR}/source/rose/makesdna
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	${CMAKE_SOURCE_DIR}/source/rose/rosekernel
	${CMAKE_SOURCE_DIR}/source/rose/windowmanager
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	ED_screen.h
	ED_space_api.h
	UI_interface.h
)



# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Executable

add_library(rf_source_editors_util INTERFACE)

target_include_directories(rf_source_editors_util INTERFACE .)
add_library(rf::source::editors::util ALIAS rf_source_editors_util)

# CMake 3.19+ allows one to populate the interface library with
# source files to show in the IDE, for people on older CMake versions
# these headers will be visible in the bf_intern_guardedalloc project
# where they historically have been.
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19")
	target_sources(rf_source_editors_util PRIVATE ${SRC})
	rose_source_group(rf_source_editors_util ${SRC})
endif()
