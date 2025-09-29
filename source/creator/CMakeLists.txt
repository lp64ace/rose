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
	creator.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::gtk
	
	rose::source::dna
	rose::source::roselib
	rose::source::rosekernel
	rose::source::rosefont
	rose::source::functions
	rose::source::gpu
	rose::source::windowmanager
	rose::source::editors::screen
	rose::source::editors::space_api
	rose::source::editors::space_statusbar
	rose::source::editors::space_topbar
	rose::source::editors::space_view3d
	rose::source::rosemesh
	rose::source::roseloader
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Executable

add_executable(rose "${SRC}")

rose_target_link_libraries(rose "${LIB}")
rose_target_include_dirs(rose "${INC}")

if(GLEW_BINARIES)
	add_custom_command(TARGET rose POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${GLEW_BINARIES}
            $<TARGET_FILE_DIR:rose>/glew32.dll
	)
endif()
