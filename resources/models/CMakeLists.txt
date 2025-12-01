# -----------------------------------------------------------------------------
# Make Resource

set(MODELS_SRC
	cat.fbx
	chief.fbx
	macarena.fbx
	six.fbx
)

set(MODELS_C
)
foreach(MODEL ${MODELS_SRC})
	data_to_c_simple(${MODEL} MODELS_C)
endforeach()

rose_add_lib(models "${MODELS_C}" "" "" "")
add_library(rose::resources::models ALIAS models)