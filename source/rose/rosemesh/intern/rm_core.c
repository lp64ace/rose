#include "MEM_guardedalloc.h"

#include "LIB_alloca.h"
#include "LIB_array.h"
#include "LIB_linklist.h"
#include "LIB_math_base.h"
#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_mempool.h"
#include "LIB_utildefines.h"

#include "DNA_meshdata_types.h"

#include "KER_customdata.h"

#include "RM_type.h"
#include "intern/rm_private.h"
#include "intern/rm_core.h"
#include "intern/rm_construct.h"
#include "intern/rm_inline.h"

#ifndef NDEBUG
// #    define USE_DEBUG_INDEX_MEMCHECK
#endif

#ifdef USE_DEBUG_INDEX_MEMCHECK
#	define DEBUG_MEMECHECK_INDEX_INVALIDATE(ele) \
		{ \
			int undef_idx; \
			RM_elem_index_set(ele, undef_idx); \
		} \
		(void)0
#endif

RMVert *RM_vert_create(RMesh *mesh, const float co[3], const RMVert *v_example, const int flag) {
	RMVert *v = LIB_memory_pool_malloc(mesh->vpool);

	ROSE_assert((v_example == NULL) || (v_example->head.htype == RM_VERT));

	v->head.data = NULL;

#ifdef USE_DEBUG_INDEX_MEMCHECK
	DEBUG_MEMECHECK_INDEX_INVALIDATE(v);
#else
	RM_elem_index_set(v, -1);
#endif

	v->head.htype = RM_VERT;
	v->head.hflag = 0;
	v->head.api_flag = 0;

	if (co) {
		copy_v3_v3(v->co, co);
	}
	else {
		zero_v3(v->co);
	}

	v->e = NULL;

	/* Disallow this flag for verts - its meaningless */
	ROSE_assert((flag & RM_CREATE_NO_DOUBLE) == 0);

	mesh->elem_index_dirty |= RM_VERT;
	mesh->elem_table_dirty |= RM_VERT;

	mesh->totvert++;

	if (flag & RM_CREATE_SKIP_CD == 0) {
		if (v_example) {
			RM_elem_attrs_copy(mesh, mesh, v_example, v);
		}
		else {
			CustomData_rmesh_set_default(&mesh->vdata, &v->head.data);
			zero_v3(v->no);
		}
	}
	else {
		if (v_example) {
			copy_v3_v3(v->no, v_example->no);
		}
		else {
			zero_v3(v->no);
		}
	}

	return v;
}

RMEdge *RM_edge_create(RMesh *mesh, RMVert *v1, RMVert *v2, const RMEdge *e_example, const int flag) {
	RMEdge *e;

	ROSE_assert(v1 != v2);
	ROSE_assert(v1->head.htype == RM_VERT && v2->head.htype == RM_VERT);
	ROSE_assert((e_example == NULL) || (e_example->head.htype == RM_EDGE));

	if ((flag & RM_CREATE_NO_DOUBLE) && (e = RM_edge_exists(v1, v2))) {
		return e;
	}

	e = LIB_memory_pool_malloc(mesh->epool);

	e->head.data = NULL;

#ifdef USE_DEBUG_INDEX_MEMCHECK
	DEBUG_MEMECHECK_INDEX_INVALIDATE(e);
#else
	RM_elem_index_set(e, -1);
#endif

	e->head.htype = RM_EDGE;
	e->head.hflag = RM_ELEM_SMOOTH | RM_ELEM_DRAW;
	e->head.api_flag = 0;

	e->v1 = v1;
	e->v2 = v2;
	e->l = NULL;

	memset(&e->v1_disk_link, 0, sizeof(RMDiskLink[2]));

	rmesh_disk_edge_append(e, e->v1);
	rmesh_disk_edge_append(e, e->v2);

	mesh->elem_index_dirty |= RM_EDGE;
	mesh->elem_table_dirty |= RM_EDGE;

	mesh->totedge++;

	if (flag & RM_CREATE_SKIP_CD == 0) {
		if (e_example) {
			RM_elem_attrs_copy(mesh, mesh, e_example, e);
		}
		else {
			CustomData_rmesh_set_default(&mesh->edata, &e->head.data);
		}
	}

	return e;
}

static RMLoop *rm_loop_create(RMesh *mesh, RMVert *v, RMEdge *e, RMFace *f, const RMLoop *l_example, const int flag) {
	RMLoop *l = NULL;

	l = LIB_memory_pool_malloc(mesh->lpool);

	ROSE_assert(RM_vert_in_edge(e, v));
	ROSE_assert(v->head.htype == RM_VERT && e->head.htype == RM_EDGE && f->head.htype == RM_FACE);
	ROSE_assert((l_example == NULL) || (l_example->head.htype == RM_EDGE));

#ifndef NDEBUG
	if (l_example) {
		/* ensure passing a loop is either sharing the same vertex, or entirely disconnected
		 * use to catch mistake passing in loop offset-by-one. */
		ROSE_assert((v == l_example->v) || !ELEM(v, l_example->prev->v, l_example->next->v));
	}
#endif

	l->head.data = NULL;

#ifdef USE_DEBUG_INDEX_MEMCHECK
	DEBUG_MEMECHECK_INDEX_INVALIDATE(l);
#else
	RM_elem_index_set(l, -1);
#endif

	l->head.htype = RM_LOOP;
	l->head.hflag = 0;
	l->head.api_flag = 0;

	l->v = v;
	l->e = e;
	l->f = f;

	l->radial_prev = NULL;
	l->radial_next = NULL;
	l->prev = NULL;
	l->next = NULL;

	mesh->elem_index_dirty |= RM_LOOP;

	mesh->totloop++;

	if (flag & RM_CREATE_SKIP_CD == 0) {
		if (l_example) {
			RM_elem_attrs_copy(mesh, mesh, l_example, l);
		}
		else {
			CustomData_rmesh_set_default(&mesh->ldata, &l->head.data);
		}
	}

	return l;
}

/**
 * only create the face, since this calloc's the length is initialized to 0,
 * leave adding loops to the caller.
 *
 * \note Caller needs to handle customdata.
 */
ROSE_INLINE RMFace *rm_face_create__internal(RMesh *mesh) {
	RMFace *f;

	f = LIB_memory_pool_calloc(mesh->fpool);

	f->head.data = NULL;

#ifdef USE_DEBUG_INDEX_MEMCHECK
	DEBUG_MEMECHECK_INDEX_INVALIDATE(f);
#else
	RM_elem_index_set(f, -1);
#endif

	f->head.htype = RM_FACE;
	f->head.hflag = 0;
	f->head.api_flag = 0;

	f->l_first = NULL;
	f->len = 0;
	f->mat_nr = 0;

	mesh->elem_index_dirty |= RM_FACE;
	mesh->elem_table_dirty |= RM_FACE;

	mesh->totface++;

	return f;
}

static RMLoop *rm_face_boundary_add(RMesh *mesh, RMFace *f, RMVert *startv, RMEdge *starte, const int flag) {
	RMLoop *l = rm_loop_create(mesh, startv, starte, f, NULL /* starte->l */, flag);

	rmesh_radial_loop_append(starte, l);

	f->l_first = l;

	return l;
}

RMFace *RM_face_create(RMesh *mesh, RMVert **verts, RMEdge **edges, const int len, const RMFace *f_example, const int flag) {
	RMFace *f = NULL;
	RMLoop *l, *startl, *lastl;
	int i;

	ROSE_assert((f_example == NULL) || (f_example->head.htype == RM_FACE));

	if (len == 0) {
		return NULL;
	}

	if (flag & RM_CREATE_NO_DOUBLE) {
		f = RM_face_exists(verts, len);
		if (f != NULL) {
			return f;
		}
	}

	f = rm_face_create__internal(mesh);

	startl = lastl = rm_face_boundary_add(mesh, f, verts[0], edges[0], flag);

	for (i = 1; i < len; i++) {
		l = rm_loop_create(mesh, verts[i], edges[i], f, NULL /* edges[i]->l */, flag);

		rmesh_radial_loop_append(edges[i], l);

		l->prev = lastl;
		lastl->next = l;
		lastl = l;
	}

	startl->prev = lastl;
	lastl->next = startl;

	f->len = len;

	if (flag & RM_CREATE_SKIP_CD) {
		if (f_example) {
			RM_elem_attrs_copy(mesh, mesh, f_example, f);
		}
		else {
			CustomData_rmesh_set_default(&mesh->pdata, &f->head.data);
			zero_v3(f->no);
		}
	}
	else {
		if (f_example) {
			copy_v3_v3(f->no, f_example->no);
		}
		else {
			zero_v3(f->no);
		}
	}

	return f;
}

RMFace *RM_face_create_verts(RMesh *mesh, RMVert **verts, const int len, const RMFace *f_example, const int flag, const bool create_edges) {
	RMEdge **edges = LIB_array_alloca(edges, len);

	if (create_edges) {
		RM_edges_from_verts_ensure(mesh, edges, verts, len);
	}
	else {
		if (RM_edges_from_verts(edges, verts, len) == false) {
			return NULL;
		}
	}

	return RM_face_create(mesh, verts, edges, len, f_example, flag);
}

RMFace *RM_face_copy(RMesh *mesh_dst, const RMesh *mesh_src, RMFace *f, bool cpy_v, bool cpy_e) {
	RMVert **verts = LIB_array_alloca(verts, f->len);
	RMEdge **edges = LIB_array_alloca(edges, f->len);
	RMLoop *l_iter;
	RMLoop *l_first;
	RMLoop *l_copy;
	RMFace *f_copy;
	int i;

	/**
	 * If we do not duplicate a face within the same mesh then we need to copy BOTH edges and
	 * vertices to the new mesh.
	 */
	ROSE_assert((mesh_dst == mesh_src) || (cpy_v && cpy_e));

	l_iter = l_first = RM_FACE_FIRST_LOOP(f);
	i = 0;
	do {
		if (cpy_v) {
			verts[i] = RM_vert_create(mesh_dst, l_iter->v->co, l_iter->v, RM_CREATE_NOP);
		}
		else {
			verts[i] = l_iter->v;
		}
		i++;
	} while ((l_iter = l_iter->next) != l_first);

	l_iter = l_first = RM_FACE_FIRST_LOOP(f);
	i = 0;
	do {
		if (cpy_e) {
			RMVert *v1, *v2;

			if (l_iter->e->v1 == verts[i]) {
				v1 = verts[i];
				v2 = verts[(i + 1) % f->len];
			}
			else {
				v2 = verts[i];
				v1 = verts[(i + 1) % f->len];
			}

			edges[i] = RM_edge_create(mesh_dst, v1, v2, l_iter->e, RM_CREATE_NOP);
		}
		else {
			edges[i] = l_iter->e;
		}
		i++;
	} while ((l_iter = l_iter->next) != l_first);

	f_copy = RM_face_create(mesh_dst, verts, edges, f->len, NULL, RM_CREATE_SKIP_CD);

	RM_elem_attrs_copy(mesh_src, mesh_dst, f, f_copy);

	l_iter = l_first = RM_FACE_FIRST_LOOP(f);
	l_copy = RM_FACE_FIRST_LOOP(f_copy);
	do {
		RM_elem_attrs_copy(mesh_src, mesh_dst, l_iter, l_copy);
		l_copy = l_copy->next;
	} while ((l_iter = l_iter->next) != l_first);

	return f_copy;
}
