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
	
	rose::source::dna
	rose::source::roselib
	rose::source::rosekernel
	rose::source::gpu
	rose::source::windowmanager
	
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
