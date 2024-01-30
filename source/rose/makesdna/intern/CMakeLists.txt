# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	PUBLIC .
	
	# Internal Include Directories
	${CMAKE_SOURCE_DIR}/source/rose/roselib
	
)

# -----------------------------------------------------------------------------
# Declare System Include Directories

set(INC_SYS
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Source Files

set(SRC
	
)



# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	
	# External Include Directories

)

# -----------------------------------------------------------------------------
# Declare Executable

add_library(rf_source_dna INTERFACE)

target_include_directories(rf_source_dna INTERFACE .)
add_library(rf::source::dna ALIAS rf_source_dna)

# CMake 3.19+ allows one to populate the interface library with
# source files to show in the IDE, for people on older CMake versions
# these headers will be visible in the bf_intern_guardedalloc project
# where they historically have been.
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.19")
	list(APPEND SRC ${DNA_TYPES_HEADERS})
	list(APPEND SRC ${DNA_DEFAULTS_HEADERS})
	
	target_sources(rf_source_dna PRIVATE ${SRC})
	rose_source_group(rf_source_dna ${SRC})
endif()
