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
	ufbx.c
	ufbx.h
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies

	# External Library Dependencies

)

# -----------------------------------------------------------------------------
# Define Library

add_definitions(
  # Turn off ufbx features that Rose does not use
  -DUFBX_NO_SUBDIVISION
  -DUFBX_NO_TESSELLATION
  -DUFBX_NO_GEOMETRY_CACHE
  -DUFBX_NO_SCENE_EVALUATION
  -DUFBX_NO_SKINNING_EVALUATION
  -DUFBX_NO_ANIMATION_BAKING
  -DUFBX_NO_TRIANGULATION
  -DUFBX_NO_INDEX_GENERATION
  -DUFBX_NO_FORMAT_OBJ
)

rose_add_lib(ufbx "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::extern::ufbx ALIAS ufbx)