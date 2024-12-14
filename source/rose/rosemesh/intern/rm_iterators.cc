#include "MEM_guardedalloc.h"

#include "LIB_bitmap.h"
#include "LIB_utildefines.h"

#include "RM_type.h"
#include "intern/rm_iterators.h"
#include "intern/rm_private.h"
#include "intern/rm_inline.h"

const char rm_iter_itype_htype_map[RM_ITYPE_MAX] = {
	'\0',
	RM_VERT, /* RM_VERTS_OF_MESH */
	RM_EDGE, /* RM_EDGES_OF_MESH */
	RM_FACE, /* RM_FACES_OF_MESH */
	RM_EDGE, /* RM_EDGES_OF_VERT */
	RM_FACE, /* RM_FACES_OF_VERT */
	RM_LOOP, /* RM_LOOPS_OF_VERT */
	RM_VERT, /* RM_VERTS_OF_EDGE */
	RM_FACE, /* RM_FACES_OF_EDGE */
	RM_VERT, /* RM_VERTS_OF_FACE */
	RM_EDGE, /* RM_EDGES_OF_FACE */
	RM_LOOP, /* RM_LOOPS_OF_FACE */
	RM_LOOP, /* RM_LOOPS_OF_LOOP */
	RM_LOOP, /* RM_LOOPS_OF_EDGE */
};

int RM_iter_mesh_count(const char itype, struct RMesh *bm) {
	int count;

	switch (itype) {
		case RM_VERTS_OF_MESH:
			count = bm->totvert;
			break;
		case RM_EDGES_OF_MESH:
			count = bm->totedge;
			break;
		case RM_FACES_OF_MESH:
			count = bm->totface;
			break;
		default:
			count = 0;
			ROSE_assert_unreachable();
			break;
	}

	return count;
}

void *RM_iter_at_index(RMesh *mesh, const char itype, void *data, int index) {
	RMIter iter;
	void *val;
	int i;

	/* sanity check */
	if (index < 0) {
		return nullptr;
	}

	val = RM_iter_new(&iter, mesh, itype, data);

	i = 0;
	while (i < index) {
		val = RM_iter_step(&iter);
		i++;
	}

	return val;
}

int RM_iter_as_array(struct RMesh *mesh, const char itype, void *data, void **array, const int len) {
	int i = 0;

	/* sanity check */
	if (len > 0) {
		RMIter iter;
		void *ele;

		for (ele = RM_iter_new(&iter, mesh, itype, data); ele; ele = RM_iter_step(&iter)) {
			array[i] = ele;
			i++;
			if (i == len) {
				return len;
			}
		}
	}

	return i;
}

void *RM_iter_as_arrayN(struct RMesh *mesh,
						const char itype,
						void *data,
						int *r_len,
						/* optional args to avoid an alloc (normally stack array) */
						void **stack_array,
						int stack_array_size) {
	RMIter iter;

	ROSE_assert(stack_array_size == 0 || (stack_array_size && stack_array));

	/* We can't rely on #RMIter.count being set. */
	switch (itype) {
		case RM_VERTS_OF_MESH:
			iter.count = mesh->totvert;
			break;
		case RM_EDGES_OF_MESH:
			iter.count = mesh->totedge;
			break;
		case RM_FACES_OF_MESH:
			iter.count = mesh->totface;
			break;
		default:
			ROSE_assert_unreachable();
			break;
	}

	if (RM_iter_init(&iter, mesh, itype, data) && iter.count > 0) {
		RMElem *ele;
		RMElem **array = iter.count > stack_array_size ? static_cast<RMElem **>(MEM_mallocN(sizeof(ele) * iter.count, __func__)) : reinterpret_cast<RMElem **>(stack_array);
		int i = 0;

		*r_len = iter.count; /* set before iterating */

		while ((ele = static_cast<RMElem *>(RM_iter_step(&iter)))) {
			array[i++] = ele;
		}
		return array;
	}

	*r_len = 0;
	return nullptr;
}

int RM_iter_mesh_bitmap_from_filter(const char itype, struct RMesh *mesh, rose::MutableBitSpan bitmap, bool (*test_fn)(RMElem *, void *user_data), void *user_data) {
	RMIter iter;
	RMElem *ele;
	int i;
	int bitmap_enabled = 0;

	RM_ITER_MESH_INDEX(ele, &iter, mesh, itype, i) {
		if (test_fn(ele, user_data)) {
			bitmap[i].set();
			bitmap_enabled++;
		}
		else {
			bitmap[i].reset();
		}
	}

	return bitmap_enabled;
}

int RM_iter_mesh_bitmap_from_filter_tessface(struct RMesh *mesh, rose::MutableBitSpan bitmap, bool (*test_fn)(RMFace *, void *user_data), void *user_data) {
	RMIter iter;
	RMFace *f;
	int i;
	int j = 0;
	int bitmap_enabled = 0;

	RM_ITER_MESH_INDEX(f, &iter, mesh, RM_FACES_OF_MESH, i) {
		if (test_fn(f, user_data)) {
			for (int tri = 2; tri < f->len; tri++) {
				bitmap[j].set();
				bitmap_enabled++;
				j++;
			}
		}
		else {
			for (int tri = 2; tri < f->len; tri++) {
				bitmap[j].reset();
				j++;
			}
		}
	}

	return bitmap_enabled;
}

int RM_iter_elem_count_flag(const char itype, void *data, const char hflag, const bool value) {
	RMIter iter;
	RMElem *ele;
	int count = 0;

	RM_ITER_ELEM(ele, &iter, data, itype) {
		if (RM_elem_flag_test_bool(ele, hflag) == value) {
			count++;
		}
	}

	return count;
}

int RM_iter_mesh_count_flag(const char itype, RMesh *bm, const char hflag, const bool value) {
	RMIter iter;
	RMElem *ele;
	int count = 0;

	RM_ITER_MESH(ele, &iter, bm, itype) {
		if (RM_elem_flag_test_bool(ele, hflag) == value) {
			count++;
		}
	}

	return count;
}

/**
 * Notes on iterator implementation:
 *
 * Iterators keep track of the next element in a sequence.
 * When a step() callback is invoked the current value of 'next'
 * is stored to be returned later and the next variable is incremented.
 *
 * When the end of a sequence is reached, next should always equal nullptr
 *
 * The 'rmiter__' prefix is used because these are used in
 * rmesh_iterators_inine.c but should otherwise be seen as
 * private.
 */

/**
 * Allow adding but not removing, this isn't _totally_ safe since
 * you could add/remove within the same loop, but catches common cases
 */
#ifndef NDEBUG
#	define USE_IMMUTABLE_ASSERT
#endif

void rmiter__elem_of_mesh_begin(struct RMIter__elem_of_mesh *iter) {
#ifdef USE_IMMUTABLE_ASSERT
	((RMIter *)iter)->count = LIB_memory_pool_length(iter->pooliter.pool);
#endif
	LIB_memory_pool_iternew(iter->pooliter.pool, &iter->pooliter);
}

void *rmiter__elem_of_mesh_step(struct RMIter__elem_of_mesh *iter) {
#ifdef USE_IMMUTABLE_ASSERT
	ROSE_assert(((RMIter *)iter)->count <= LIB_memory_pool_length(iter->pooliter.pool));
#endif
	return LIB_memory_pool_iterstep(&iter->pooliter);
}

#ifdef USE_IMMUTABLE_ASSERT
#	undef USE_IMMUTABLE_ASSERT
#endif

/*
 * EDGE OF VERT CALLBACKS
 */

void rmiter__edge_of_vert_begin(struct RMIter__edge_of_vert *iter) {
	if (iter->vdata->e) {
		iter->e_first = iter->vdata->e;
		iter->e_next = iter->vdata->e;
	}
	else {
		iter->e_first = nullptr;
		iter->e_next = nullptr;
	}
}

void *rmiter__edge_of_vert_step(struct RMIter__edge_of_vert *iter) {
	RMEdge *e_curr = iter->e_next;

	if (iter->e_next) {
		iter->e_next = rmesh_disk_edge_next(iter->e_next, iter->vdata);
		if (iter->e_next == iter->e_first) {
			iter->e_next = nullptr;
		}
	}

	return e_curr;
}

/*
 * FACE OF VERT CALLBACKS
 */

void rmiter__face_of_vert_begin(struct RMIter__face_of_vert *iter) {
	((RMIter *)iter)->count = rmesh_disk_facevert_count(iter->vdata);
	if (((RMIter *)iter)->count) {
		iter->l_first = rmesh_disk_faceloop_find_first(iter->vdata->e, iter->vdata);
		iter->e_first = iter->l_first->e;
		iter->e_next = iter->e_first;
		iter->l_next = iter->l_first;
	}
	else {
		iter->l_first = iter->l_next = nullptr;
		iter->e_first = iter->e_next = nullptr;
	}
}
void *rmiter__face_of_vert_step(struct RMIter__face_of_vert *iter) {
	RMLoop *l_curr = iter->l_next;

	if (((RMIter *)iter)->count && iter->l_next) {
		((RMIter *)iter)->count--;
		iter->l_next = rmesh_radial_faceloop_find_next(iter->l_next, iter->vdata);
		if (iter->l_next == iter->l_first) {
			iter->e_next = rmesh_disk_faceedge_find_next(iter->e_next, iter->vdata);
			iter->l_first = rmesh_radial_faceloop_find_first(iter->e_next->l, iter->vdata);
			iter->l_next = iter->l_first;
		}
	}

	if (!((RMIter *)iter)->count) {
		iter->l_next = nullptr;
	}

	return l_curr ? l_curr->f : nullptr;
}

/*
 * LOOP OF VERT CALLBACKS
 */

void rmiter__loop_of_vert_begin(struct RMIter__loop_of_vert *iter) {
	((RMIter *)iter)->count = rmesh_disk_facevert_count(iter->vdata);
	if (((RMIter *)iter)->count) {
		iter->l_first = rmesh_disk_faceloop_find_first(iter->vdata->e, iter->vdata);
		iter->e_first = iter->l_first->e;
		iter->e_next = iter->e_first;
		iter->l_next = iter->l_first;
	}
	else {
		iter->l_first = iter->l_next = nullptr;
		iter->e_first = iter->e_next = nullptr;
	}
}
void *rmiter__loop_of_vert_step(struct RMIter__loop_of_vert *iter) {
	RMLoop *l_curr = iter->l_next;

	if (((RMIter *)iter)->count) {
		((RMIter *)iter)->count--;
		iter->l_next = rmesh_radial_faceloop_find_next(iter->l_next, iter->vdata);
		if (iter->l_next == iter->l_first) {
			iter->e_next = rmesh_disk_faceedge_find_next(iter->e_next, iter->vdata);
			iter->l_first = rmesh_radial_faceloop_find_first(iter->e_next->l, iter->vdata);
			iter->l_next = iter->l_first;
		}
	}

	if (!((RMIter *)iter)->count) {
		iter->l_next = nullptr;
	}

	/* nullptr on finish */
	return l_curr;
}

/*
 * LOOP OF EDGE CALLBACKS
 */

void rmiter__loop_of_edge_begin(struct RMIter__loop_of_edge *iter) {
	iter->l_first = iter->l_next = iter->edata->l;
}

void *rmiter__loop_of_edge_step(struct RMIter__loop_of_edge *iter) {
	RMLoop *l_curr = iter->l_next;

	if (iter->l_next) {
		iter->l_next = iter->l_next->radial_next;
		if (iter->l_next == iter->l_first) {
			iter->l_next = nullptr;
		}
	}

	/* nullptr on finish */
	return l_curr;
}

/*
 * LOOP OF LOOP CALLBACKS
 */

void rmiter__loop_of_loop_begin(struct RMIter__loop_of_loop *iter) {
	iter->l_first = iter->ldata;
	iter->l_next = iter->l_first->radial_next;

	if (iter->l_next == iter->l_first) {
		iter->l_next = nullptr;
	}
}

void *rmiter__loop_of_loop_step(struct RMIter__loop_of_loop *iter) {
	RMLoop *l_curr = iter->l_next;

	if (iter->l_next) {
		iter->l_next = iter->l_next->radial_next;
		if (iter->l_next == iter->l_first) {
			iter->l_next = nullptr;
		}
	}

	/* nullptr on finish */
	return l_curr;
}

/*
 * FACE OF EDGE CALLBACKS
 */

void rmiter__face_of_edge_begin(struct RMIter__face_of_edge *iter) {
	iter->l_first = iter->l_next = iter->edata->l;
}

void *rmiter__face_of_edge_step(struct RMIter__face_of_edge *iter) {
	RMLoop *current = iter->l_next;

	if (iter->l_next) {
		iter->l_next = iter->l_next->radial_next;
		if (iter->l_next == iter->l_first) {
			iter->l_next = nullptr;
		}
	}

	return current ? current->f : nullptr;
}

/*
 * VERTS OF EDGE CALLBACKS
 */

void rmiter__vert_of_edge_begin(struct RMIter__vert_of_edge *iter) {
	((RMIter *)iter)->count = 0;
}

void *rmiter__vert_of_edge_step(struct RMIter__vert_of_edge *iter) {
	switch (((RMIter *)iter)->count++) {
		case 0:
			return iter->edata->v1;
		case 1:
			return iter->edata->v2;
		default:
			return nullptr;
	}
}

/*
 * VERT OF FACE CALLBACKS
 */

void rmiter__vert_of_face_begin(struct RMIter__vert_of_face *iter) {
	iter->l_first = iter->l_next = RM_FACE_FIRST_LOOP(iter->pdata);
}

void *rmiter__vert_of_face_step(struct RMIter__vert_of_face *iter) {
	RMLoop *l_curr = iter->l_next;

	if (iter->l_next) {
		iter->l_next = iter->l_next->next;
		if (iter->l_next == iter->l_first) {
			iter->l_next = nullptr;
		}
	}

	return l_curr ? l_curr->v : nullptr;
}

/*
 * EDGE OF FACE CALLBACKS
 */

void rmiter__edge_of_face_begin(struct RMIter__edge_of_face *iter) {
	iter->l_first = iter->l_next = RM_FACE_FIRST_LOOP(iter->pdata);
}

void *rmiter__edge_of_face_step(struct RMIter__edge_of_face *iter) {
	RMLoop *l_curr = iter->l_next;

	if (iter->l_next) {
		iter->l_next = iter->l_next->next;
		if (iter->l_next == iter->l_first) {
			iter->l_next = nullptr;
		}
	}

	return l_curr ? l_curr->e : nullptr;
}

/*
 * LOOP OF FACE CALLBACKS
 */

void rmiter__loop_of_face_begin(struct RMIter__loop_of_face *iter) {
	iter->l_first = iter->l_next = RM_FACE_FIRST_LOOP(iter->pdata);
}

void *rmiter__loop_of_face_step(struct RMIter__loop_of_face *iter) {
	RMLoop *l_curr = iter->l_next;

	if (iter->l_next) {
		iter->l_next = iter->l_next->next;
		if (iter->l_next == iter->l_first) {
			iter->l_next = nullptr;
		}
	}

	return l_curr;
}
