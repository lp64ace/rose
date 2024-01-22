# -----------------------------------------------------------------------------
# Declare Include Directories

set(INC
	# Common Include Directories
	.
	
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
	creator.c
)

# -----------------------------------------------------------------------------
# Declare Library Dependencies

set(LIB
	# Internal Include Directories
	rf::intern::memalloc
	rf::source::roselib
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Executable

add_executable(rf_source_rose ${EXETYPE} ${SRC})

target_link_libraries(rf_source_rose ${LIB})

rose_target_include_dirs(rf_source_rose ${INC})
rose_source_group(rf_source_rose ${SRC})
