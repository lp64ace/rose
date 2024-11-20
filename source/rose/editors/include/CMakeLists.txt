# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	ED_screen.h
	ED_space_api.h
	UI_interface.h
	
)

# -----------------------------------------------------------------------------
# Declare Library

add_library(include INTERFACE "${SRC}")
add_library(rose::source::editors::include ALIAS include)

rose_source_group(include "${SRC}")
