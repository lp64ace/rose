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
	${PTHREADS_INCLUDE_DIRS}
	
)

# -----------------------------------------------------------------------------
# Define Source Files

set(SRC
	KER_action.h
	KER_action.hh
	KER_anim_data.h
	KER_armature.h
	KER_context.h
	KER_cpp_types.h
	KER_collection.h
	KER_customdata.h
	KER_deform.h
	KER_fcurve.h
	KER_global.h
	KER_idprop.h
	KER_idtype.h
	KER_layer.h
	KER_library.h
	KER_lib_id.h
	KER_lib_query.h
	KER_lib_remap.h
	KER_main.h
	KER_mesh.h
	KER_mesh.hh
	KER_mesh_types.hh
	KER_modifier.h
	KER_object.h
	KER_object_deform.h
	KER_rose.h
	KER_rosefile.h
	KER_scene.h
	KER_screen.h
	KER_userdef.h

	intern/action.c
	intern/action_runtime.cc
	intern/anim_data.c
	intern/armature.c
	intern/armature_deform.cc
	intern/context.c
	intern/cpp_types.cc
	intern/collection.c
	intern/customdata.cc
	intern/deform.c
	intern/fcurve.c
	intern/idprop.c
	intern/idtype.c
	intern/layer.c
	intern/library.c
	intern/lib_id.c
	intern/lib_query.c
	intern/lib_remap.c
	intern/main.c
	intern/mesh.c
	intern/mesh.cc
	intern/mesh_data_update.c
	intern/mesh_normals.cc
	intern/mesh_runtime.cc
	intern/mesh_tessellate.cc
	intern/modifier.c
	intern/object.c
	intern/object_deform.c
	intern/rose.c
	intern/rosefile.c
	intern/scene.c
	intern/screen.c
	intern/userdef.c
	
)

# -----------------------------------------------------------------------------
# Define Library Dependencies

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::intern::atomic
	PUBLIC rose::source::roselib
	PUBLIC rose::source::dna
	PUBLIC rose::source::rna
	PUBLIC rose::source::functions
	PUBLIC rose::source::modifiers
	PUBLIC rose::source::editors::interface
	
	# External Library Dependencies
	
)

# -----------------------------------------------------------------------------
# Declare Library

rose_add_lib(rosekernel "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
add_library(rose::source::rosekernel ALIAS rosekernel)

# -----------------------------------------------------------------------------
# Define Include Directories (Test)

set(INC
	# Internal Include Directories
	PUBLIC .
	
	# External Include Directories
	
)

# -----------------------------------------------------------------------------
# Define System Include Directories (Test)

set(INC_SYS
	# External System Include Directories
	
)

# -----------------------------------------------------------------------------
# Define Source Files (Test)

set(TEST
	test/lib_id_free.cc
	test/lib_remap.cc
	test/mesh.cc
)

# -----------------------------------------------------------------------------
# Define Library Dependencies (Test)

set(LIB
	# Internal Library Dependencies
	rose::intern::guardedalloc
	rose::source::roselib
	rose::source::dna
	rose::source::rosemesh
	rose::source::rosekernel
	rose::source::windowmanager
	
	# External Library Dependencies
	${PTHREADS_LIBRARIES}
	
)

# -----------------------------------------------------------------------------
# Declare Test

rose_add_test_executable(rosekernel "${TEST}" "${INC}" "${INC_SYS}" "${LIB}")

