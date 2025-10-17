# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	ED_armature.h
	ED_screen.h
	ED_space_api.h
	UI_interface.h
	UI_resource.h
	UI_view2d.h
	
)

# -----------------------------------------------------------------------------
# Declare Library

add_library(include INTERFACE "${SRC}")
add_library(rose::source::editors::include ALIAS include)

rose_source_group(include "${SRC}")
