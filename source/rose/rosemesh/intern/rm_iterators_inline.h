#ifndef RM_ITERATORS_INLINE_H
#define RM_ITERATORS_INLINE_H

#include "RM_type.h"

/**
 * \brief Iterator Step
 *
 * Calls an iterators step function to return the next element.
 */
ROSE_INLINE void *RM_iter_step(struct RMIter *iter) {
	return iter->step(iter);
}

/**
 * \brief Iterator Init
 *
 * Takes a bmesh iterator structure and fills
 * it with the appropriate function pointers based
 * upon its type.
 */
ROSE_INLINE bool RM_iter_init(struct RMIter *iter, struct RMesh *mesh, const char itype, void *data) {
	/* int argtype; */
	iter->itype = itype;

	/* inlining optimizes out this switch when called with the defined type */
	switch ((RMIterType)itype) {
		case RM_VERTS_OF_MESH:
			ROSE_assert(mesh != NULL);
			ROSE_assert(data == NULL);
			iter->begin = (RMIter__begin_cb)rmiter__elem_of_mesh_begin;
			iter->step = (RMIter__step_cb)rmiter__elem_of_mesh_step;
			iter->data.elem_of_mesh.pooliter.pool = mesh->vpool;
			break;
		case RM_EDGES_OF_MESH:
			ROSE_assert(mesh != NULL);
			ROSE_assert(data == NULL);
			iter->begin = (RMIter__begin_cb)rmiter__elem_of_mesh_begin;
			iter->step = (RMIter__step_cb)rmiter__elem_of_mesh_step;
			iter->data.elem_of_mesh.pooliter.pool = mesh->epool;
			break;
		case RM_FACES_OF_MESH:
			ROSE_assert(mesh != NULL);
			ROSE_assert(data == NULL);
			iter->begin = (RMIter__begin_cb)rmiter__elem_of_mesh_begin;
			iter->step = (RMIter__step_cb)rmiter__elem_of_mesh_step;
			iter->data.elem_of_mesh.pooliter.pool = mesh->fpool;
			break;
		case RM_EDGES_OF_VERT:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_VERT);
			iter->begin = (RMIter__begin_cb)rmiter__edge_of_vert_begin;
			iter->step = (RMIter__step_cb)rmiter__edge_of_vert_step;
			iter->data.edge_of_vert.vdata = (struct RMVert *)data;
			break;
		case RM_FACES_OF_VERT:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_VERT);
			iter->begin = (RMIter__begin_cb)rmiter__face_of_vert_begin;
			iter->step = (RMIter__step_cb)rmiter__face_of_vert_step;
			iter->data.face_of_vert.vdata = (struct RMVert *)data;
			break;
		case RM_LOOPS_OF_VERT:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_VERT);
			iter->begin = (RMIter__begin_cb)rmiter__loop_of_vert_begin;
			iter->step = (RMIter__step_cb)rmiter__loop_of_vert_step;
			iter->data.loop_of_vert.vdata = (struct RMVert *)data;
			break;
		case RM_VERTS_OF_EDGE:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_EDGE);
			iter->begin = (RMIter__begin_cb)rmiter__vert_of_edge_begin;
			iter->step = (RMIter__step_cb)rmiter__vert_of_edge_step;
			iter->data.vert_of_edge.edata = (struct RMEdge *)data;
			break;
		case RM_FACES_OF_EDGE:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_EDGE);
			iter->begin = (RMIter__begin_cb)rmiter__face_of_edge_begin;
			iter->step = (RMIter__step_cb)rmiter__face_of_edge_step;
			iter->data.face_of_edge.edata = (struct RMEdge *)data;
			break;
		case RM_VERTS_OF_FACE:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_FACE);
			iter->begin = (RMIter__begin_cb)rmiter__vert_of_face_begin;
			iter->step = (RMIter__step_cb)rmiter__vert_of_face_step;
			iter->data.vert_of_face.pdata = (struct RMFace *)data;
			break;
		case RM_EDGES_OF_FACE:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_FACE);
			iter->begin = (RMIter__begin_cb)rmiter__edge_of_face_begin;
			iter->step = (RMIter__step_cb)rmiter__edge_of_face_step;
			iter->data.edge_of_face.pdata = (struct RMFace *)data;
			break;
		case RM_LOOPS_OF_FACE:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_FACE);
			iter->begin = (RMIter__begin_cb)rmiter__loop_of_face_begin;
			iter->step = (RMIter__step_cb)rmiter__loop_of_face_step;
			iter->data.loop_of_face.pdata = (struct RMFace *)data;
			break;
		case RM_LOOPS_OF_LOOP:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_LOOP);
			iter->begin = (RMIter__begin_cb)rmiter__loop_of_loop_begin;
			iter->step = (RMIter__step_cb)rmiter__loop_of_loop_step;
			iter->data.loop_of_loop.ldata = (struct RMLoop *)data;
			break;
		case RM_LOOPS_OF_EDGE:
			ROSE_assert(data != NULL);
			ROSE_assert(((struct RMElem *)data)->head.htype == RM_EDGE);
			iter->begin = (RMIter__begin_cb)rmiter__loop_of_edge_begin;
			iter->step = (RMIter__step_cb)rmiter__loop_of_edge_step;
			iter->data.loop_of_edge.edata = (struct RMEdge *)data;
			break;
		default:
			/* should never happen */
			ROSE_assert_unreachable();
			return false;
	}

	iter->begin(iter);

	return true;
}

/**
 * \brief Iterator New
 *
 * Takes a mesh iterator structure and fills it with the appropriate function pointers based upon
 * its type and then calls #RMeshIter_step() to return the first element of the iterator.
 */
ROSE_INLINE void *RM_iter_new(RMIter *iter, RMesh *mesh, const char itype, void *data) {
	if (RM_iter_init(iter, mesh, itype, data)) {
		return RM_iter_step(iter);
	}
	else {
		return NULL;
	}
}

#endif // RM_ITERATORS_INLINE_H
