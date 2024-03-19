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
	rf::intern::ghost
	rf::source::editors::interface
	rf::source::editors::screen
	rf::source::editors::space_api
	rf::source::editors::space_statusbar
	rf::source::editors::space_topbar
	rf::source::editors::space_view3d
	rf::source::gpu
	rf::source::rosekernel
	rf::source::roselib
	rf::source::windowmanager
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Declare Executable

add_executable(rose ${EXETYPE} ${SRC})

target_link_libraries(rose ${LIB})

rose_target_include_dirs(rose ${INC})
rose_source_group(rose ${SRC})

# Ugly but overrides the folder set by `rose_source_group`
set_target_properties(rose PROPERTIES FOLDER "")
