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
	rose::intern::native
	
	rose::source::dna
	rose::source::rna
	rose::source::roselib
	rose::source::rosekernel
	rose::source::rosefont
	rose::source::functions
	rose::source::gpu
	rose::source::io::fbx
	rose::source::windowmanager
	rose::source::editors::screen
	rose::source::editors::space_api
	rose::source::editors::space_statusbar
	rose::source::editors::space_topbar
	rose::source::editors::space_view3d
	rose::source::editors::space_properties
	rose::source::rosemesh
	rose::source::roseloader

	rose::resources::models
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Executable

add_executable(rose "${SRC}")

rose_target_link_libraries(rose "${LIB}")
rose_target_include_dirs(rose "${INC}")

if(GLEW_BINARIES)
	foreach(BINARY_PATH ${GLEW_BINARIES})
		get_filename_component(BINARY_NAME ${BINARY_PATH} NAME)
		add_custom_command(TARGET rose POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${BINARY_PATH}
				$<TARGET_FILE_DIR:rose>/${BINARY_NAME}
		)
	endforeach()
endif()

if(TBB_BINARIES)
	foreach(BINARY_PATH ${TBB_BINARIES})
		get_filename_component(BINARY_NAME ${BINARY_PATH} NAME)
		add_custom_command(TARGET rose POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different
				${BINARY_PATH}
				$<TARGET_FILE_DIR:rose>/${BINARY_NAME}
		)
	endforeach()
endif()
