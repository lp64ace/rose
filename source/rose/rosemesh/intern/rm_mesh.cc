#include "MEM_guardedalloc.h"

#include "DNA_listbase.h"

#include "LIB_listbase.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "KER_customdata.h"

#include "rm_mesh.h"

#include "intern/rm_iterators.h"

const size_t rm_mesh_allocsize_default_verts = 512;
const size_t rm_mesh_allocsize_default_edges = 1024;
const size_t rm_mesh_allocsize_default_loops = 2048;
const size_t rm_mesh_allocsize_default_faces = 512;

const size_t rm_mesh_chunksize_default_verts = 512;
const size_t rm_mesh_chunksize_default_edges = 1024;
const size_t rm_mesh_chunksize_default_loops = 2048;
const size_t rm_mesh_chunksize_default_faces = 512;

static void rm_mempool_init_ex(const bool use_toolflags, MemPool **r_vpool, MemPool **r_epool, MemPool **r_lpool, MemPool **r_fpool) {
	size_t vert_size, edge_size, loop_size, face_size;

	if (use_toolflags == false) {
		vert_size = sizeof(RMVert);
		edge_size = sizeof(RMEdge);
		loop_size = sizeof(RMLoop);
		face_size = sizeof(RMFace);
	}
	else {
		vert_size = sizeof(RMVert_OFlag);
		edge_size = sizeof(RMEdge_OFlag);
		loop_size = sizeof(RMLoop);
		face_size = sizeof(RMFace_OFlag);
	}

	if (r_vpool) {
		*r_vpool = LIB_memory_pool_create(vert_size, rm_mesh_allocsize_default_verts, rm_mesh_chunksize_default_verts, ROSE_MEMPOOL_ALLOW_ITER);
	}
	if (r_epool) {
		*r_epool = LIB_memory_pool_create(edge_size, rm_mesh_allocsize_default_edges, rm_mesh_chunksize_default_edges, ROSE_MEMPOOL_ALLOW_ITER);
	}
	if (r_lpool) {
		*r_lpool = LIB_memory_pool_create(loop_size, rm_mesh_allocsize_default_loops, rm_mesh_chunksize_default_loops, ROSE_MEMPOOL_ALLOW_ITER);
	}
	if (r_fpool) {
		*r_fpool = LIB_memory_pool_create(face_size, rm_mesh_allocsize_default_faces, rm_mesh_chunksize_default_faces, ROSE_MEMPOOL_ALLOW_ITER);
	}
}

static void rm_mempool_init(RMesh *mesh, const bool use_toolflags) {
	rm_mempool_init_ex(use_toolflags, &mesh->vpool, &mesh->epool, &mesh->lpool, &mesh->fpool);
}

RMesh *RM_mesh_create(void) {
	/** Allocate the structure. */
	RMesh *mesh = static_cast<RMesh *>(MEM_callocN(sizeof(RMesh), __func__));

	/** Allocate the memory pools for the mesh elements. */
	rm_mempool_init(mesh, false);

	CustomData_reset(&mesh->vdata);
	CustomData_reset(&mesh->edata);
	CustomData_reset(&mesh->ldata);
	CustomData_reset(&mesh->pdata);

	return mesh;
}

void RM_mesh_data_free(RMesh *mesh) {
	RMVert *v;
	RMEdge *e;
	RMLoop *l;
	RMFace *f;

	RMIter iter;
	RMIter itersub;

	const bool is_ldata_free = CustomData_rmesh_has_free(&mesh->ldata);
	const bool is_pdata_free = CustomData_rmesh_has_free(&mesh->pdata);

	/* Check if we have to call free, if not we can avoid a lot of looping */
	if (CustomData_rmesh_has_free(&(mesh->vdata))) {
		RM_ITER_MESH(v, &iter, mesh, RM_VERTS_OF_MESH) {
			CustomData_rmesh_free_block(&(mesh->vdata), &(v->head.data));
		}
	}
	if (CustomData_rmesh_has_free(&(mesh->edata))) {
		RM_ITER_MESH(e, &iter, mesh, RM_EDGES_OF_MESH) {
			CustomData_rmesh_free_block(&(mesh->edata), &(e->head.data));
		}
	}

	if (is_ldata_free || is_pdata_free) {
		RM_ITER_MESH(f, &iter, mesh, RM_FACES_OF_MESH) {
			if (is_pdata_free) {
				CustomData_rmesh_free_block(&(mesh->pdata), &(f->head.data));
			}
			if (is_ldata_free) {
				RM_ITER_ELEM(l, &itersub, f, RM_LOOPS_OF_FACE) {
					CustomData_rmesh_free_block(&(mesh->ldata), &(l->head.data));
				}
			}
		}
	}

	/* Free custom data pools, This should probably go in CustomData_free? */
	if (mesh->vdata.totlayer) {
		LIB_memory_pool_destroy(mesh->vdata.pool);
	}
	if (mesh->edata.totlayer) {
		LIB_memory_pool_destroy(mesh->edata.pool);
	}
	if (mesh->ldata.totlayer) {
		LIB_memory_pool_destroy(mesh->ldata.pool);
	}
	if (mesh->pdata.totlayer) {
		LIB_memory_pool_destroy(mesh->pdata.pool);
	}

	/* free custom data */
	CustomData_free(&mesh->vdata, 0);
	CustomData_free(&mesh->edata, 0);
	CustomData_free(&mesh->ldata, 0);
	CustomData_free(&mesh->pdata, 0);

	/* destroy element pools */
	LIB_memory_pool_destroy(mesh->vpool);
	LIB_memory_pool_destroy(mesh->epool);
	LIB_memory_pool_destroy(mesh->lpool);
	LIB_memory_pool_destroy(mesh->fpool);

	if (mesh->vtable) {
		MEM_freeN(mesh->vtable);
	}
	if (mesh->etable) {
		MEM_freeN(mesh->etable);
	}
	if (mesh->ftable) {
		MEM_freeN(mesh->ftable);
	}
}

void RM_mesh_clear(RMesh *mesh) {
	/* free old mesh */
	RM_mesh_data_free(mesh);
	memset(mesh, 0, sizeof(RMesh));

	/* allocate the memory pools for the mesh elements */
	rm_mempool_init(mesh, false);

	CustomData_reset(&mesh->vdata);
	CustomData_reset(&mesh->edata);
	CustomData_reset(&mesh->ldata);
	CustomData_reset(&mesh->pdata);
}

void RM_mesh_free(RMesh *mesh) {
	RM_mesh_data_free(mesh);

	MEM_freeN(mesh);
}

bool RM_mesh_elem_table_check(RMesh *mesh) {
	RMIter iter;
	RMElem *ele;
	int i;

	if (mesh->vtable && ((mesh->elem_table_dirty & RM_VERT) == 0)) {
		RM_ITER_MESH_INDEX(ele, &iter, mesh, RM_VERTS_OF_MESH, i) {
			if (ele != (RMElem *)mesh->vtable[i]) {
				return false;
			}
		}
	}

	if (mesh->etable && ((mesh->elem_table_dirty & RM_EDGE) == 0)) {
		RM_ITER_MESH_INDEX(ele, &iter, mesh, RM_EDGES_OF_MESH, i) {
			if (ele != (RMElem *)mesh->etable[i]) {
				return false;
			}
		}
	}

	if (mesh->ftable && ((mesh->elem_table_dirty & RM_FACE) == 0)) {
		RM_ITER_MESH_INDEX(ele, &iter, mesh, RM_FACES_OF_MESH, i) {
			if (ele != (RMElem *)mesh->ftable[i]) {
				return false;
			}
		}
	}

	return true;
}

void RM_mesh_elem_table_ensure(RMesh *mesh, const char htype) {
	/* assume if the array is non-null then its valid and no need to recalc */
	const char htype_needed = (((mesh->vtable && ((mesh->elem_table_dirty & RM_VERT) == 0)) ? 0 : RM_VERT) | ((mesh->etable && ((mesh->elem_table_dirty & RM_EDGE) == 0)) ? 0 : RM_EDGE) | ((mesh->ftable && ((mesh->elem_table_dirty & RM_FACE) == 0)) ? 0 : RM_FACE)) & htype;

	ROSE_assert((htype & ~RM_ALL_NOLOOP) == 0);

	/* in debug mode double check we didn't need to recalculate */
	ROSE_assert(RM_mesh_elem_table_check(mesh) == true);

	if (htype_needed) {
		if (htype_needed & RM_VERT) {
			if (mesh->vtable && mesh->totvert <= mesh->vtable_tot && mesh->totvert * 2 >= mesh->vtable_tot) {
				/* pass (re-use the array) */
			}
			else {
				if (mesh->vtable) {
					MEM_freeN(mesh->vtable);
				}
				mesh->vtable = static_cast<RMVert **>(MEM_mallocN(sizeof(void **) * mesh->totvert, "mesh->vtable"));
				mesh->vtable_tot = mesh->totvert;
			}
			RM_iter_as_array(mesh, RM_VERTS_OF_MESH, nullptr, (void **)mesh->vtable, mesh->totvert);
		}
		if (htype_needed & RM_EDGE) {
			if (mesh->etable && mesh->totedge <= mesh->etable_tot && mesh->totedge * 2 >= mesh->etable_tot) {
				/* pass (re-use the array) */
			}
			else {
				if (mesh->etable) {
					MEM_freeN(mesh->etable);
				}
				mesh->etable = static_cast<RMEdge **>(MEM_mallocN(sizeof(void **) * mesh->totedge, "mesh->etable"));
				mesh->etable_tot = mesh->totedge;
			}
			RM_iter_as_array(mesh, RM_EDGES_OF_MESH, nullptr, (void **)mesh->etable, mesh->totedge);
		}
		if (htype_needed & RM_FACE) {
			if (mesh->ftable && mesh->totface <= mesh->ftable_tot && mesh->totface * 2 >= mesh->ftable_tot) {
				/* pass (re-use the array) */
			}
			else {
				if (mesh->ftable) {
					MEM_freeN(mesh->ftable);
				}
				mesh->ftable = static_cast<RMFace **>(MEM_mallocN(sizeof(void **) * mesh->totface, "mesh->ftable"));
				mesh->ftable_tot = mesh->totface;
			}
			RM_iter_as_array(mesh, RM_FACES_OF_MESH, nullptr, (void **)mesh->ftable, mesh->totface);
		}
	}

	/* Only clear dirty flags when all the pointers and data are actually valid.
	 * This prevents possible threading issues when dirty flag check failed but
	 * data wasn't ready still.
	 */
	mesh->elem_table_dirty &= ~htype_needed;
}

void RM_mesh_elem_table_free(RMesh *mesh, const char htype) {
	if (htype & RM_VERT) {
		MEM_SAFE_FREE(mesh->vtable);
	}

	if (htype & RM_EDGE) {
		MEM_SAFE_FREE(mesh->etable);
	}

	if (htype & RM_FACE) {
		MEM_SAFE_FREE(mesh->ftable);
	}
}

void RM_mesh_elem_table_init(RMesh *mesh, const char htype) {
	ROSE_assert((htype & ~RM_ALL_NOLOOP) == 0);

	/* force recalc */
	RM_mesh_elem_table_free(mesh, RM_ALL_NOLOOP);
	RM_mesh_elem_table_ensure(mesh, htype);
}

RMVert *RM_vert_at_index_find(RMesh *mesh, const int index) {
	return static_cast<RMVert *>(LIB_memory_pool_findelem(mesh->vpool, index));
}

RMEdge *RM_edge_at_index_find(RMesh *mesh, const int index) {
	return static_cast<RMEdge *>(LIB_memory_pool_findelem(mesh->epool, index));
}

RMFace *RM_face_at_index_find(RMesh *mesh, const int index) {
	return static_cast<RMFace *>(LIB_memory_pool_findelem(mesh->fpool, index));
}

RMLoop *RM_loop_at_index_find(RMesh *mesh, const int index) {
	RMIter iter;
	RMFace *f;
	int i = index;
	RM_ITER_MESH(f, &iter, mesh, RM_FACES_OF_MESH) {
		if (i < f->len) {
			RMLoop *l_first, *l_iter;
			l_iter = l_first = RM_FACE_FIRST_LOOP(f);
			do {
				if (i == 0) {
					return l_iter;
				}
				i -= 1;
			} while ((l_iter = l_iter->next) != l_first);
		}
		i -= f->len;
	}
	return nullptr;
}

RMVert *RM_vert_at_index_find_or_table(RMesh *mesh, const int index) {
	if ((mesh->elem_table_dirty & RM_VERT) == 0) {
		return (index < mesh->totvert) ? mesh->vtable[index] : nullptr;
	}
	return RM_vert_at_index_find(mesh, index);
}

RMEdge *RM_edge_at_index_find_or_table(RMesh *mesh, const int index) {
	if ((mesh->elem_table_dirty & RM_EDGE) == 0) {
		return (index < mesh->totedge) ? mesh->etable[index] : nullptr;
	}
	return RM_edge_at_index_find(mesh, index);
}

RMFace *RM_face_at_index_find_or_table(RMesh *mesh, const int index) {
	if ((mesh->elem_table_dirty & RM_FACE) == 0) {
		return (index < mesh->totface) ? mesh->ftable[index] : nullptr;
	}
	return RM_face_at_index_find(mesh, index);
}

int RM_mesh_elem_count(RMesh *mesh, const char htype) {
	ROSE_assert((htype & ~RM_ALL_NOLOOP) == 0);

	switch (htype) {
		case RM_VERT:
			return mesh->totvert;
		case RM_EDGE:
			return mesh->totedge;
		case RM_FACE:
			return mesh->totface;
		default: {
			ROSE_assert_unreachable();
			return 0;
		}
	}
}
