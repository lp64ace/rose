#include "MEM_guardedalloc.h"

#include "LIB_thread.h"

#include "KER_mesh.h"

/* -------------------------------------------------------------------- */
/** \name Mesh Runtime Struct Utils
 * \{ */

/**
 * \brief Initialize the runtime mutexes of the given mesh.
 *
 * Any existing mutexes will be overridden.
 */
ROSE_STATIC void mesh_runtime_init_mutexes(Mesh *mesh) {
	mesh->runtime.normals_mutex = MEM_callocN(sizeof(ThreadMutex), "MeshRuntime::normals_mutex");
	LIB_mutex_init(mesh->runtime.normals_mutex);
}

/**
 * \brief free the mutexes of the given mesh runtime.
 */
ROSE_STATIC void mesh_runtime_free_mutexes(Mesh *mesh) {
	if (mesh->runtime.normals_mutex != NULL) {
		LIB_mutex_end((ThreadMutex *)mesh->runtime.normals_mutex);
		MEM_freeN(mesh->runtime.normals_mutex);
		mesh->runtime.normals_mutex = NULL;
	}
}

void KER_mesh_runtime_init_data(Mesh *mesh) {
	mesh_runtime_init_mutexes(mesh);
}

void KER_mesh_runtime_free_data(Mesh *mesh) {
	KER_mesh_runtime_clear_cache(mesh);
	mesh_runtime_free_mutexes(mesh);
}

void KER_mesh_runtime_clear_cache(Mesh *mesh) {
	KER_mesh_runtime_clear_geometry(mesh);
	KER_mesh_clear_derived_normals(mesh);
}

void KER_mesh_runtime_clear_geometry(Mesh *mesh) {
	MEM_SAFE_FREE(mesh->runtime.looptris.array);
}

/** \} */
