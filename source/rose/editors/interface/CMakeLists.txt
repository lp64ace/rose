# -----------------------------------------------------------------------------
# Define Include Directories

set(INC
	# Internal Include Directories
	PUBLIC ../include
	
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
	interface_intern.h
	
	interface.c
	interface_button.c
	interface_draw.c
	interface_handlers.c
	interface_layout.cc
	interface_region.c
	interface_region_popup.c
	interface_undo.c
	interface_widgets.c
	resources.c
	
	intern/userdef_theme_default.c
	intern/userdef_theme_light.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::rosekernel
	rose::source::rosefont
	rose::source::gpu
	rose::source::windowmanager
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(interface "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::editors::interface ALIAS interface)
