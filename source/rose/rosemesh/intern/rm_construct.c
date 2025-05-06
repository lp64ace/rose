#include "MEM_guardedalloc.h"

#include "LIB_math_matrix.h"
#include "LIB_math_vector.h"
#include "LIB_sys_types.h"
#include "LIB_sort_utils.h"

#include "KER_customdata.h"

#include "DNA_customdata_types.h"
#include "DNA_meshdata_types.h"

#include "RM_type.h"
#include "intern/rm_private.h"
#include "intern/rm_core.h"

bool RM_verts_from_edges(RMVert **vert_arr, RMEdge **edge_arr, const int len) {
	int i, i_prev = len - 1;
	for (i = 0; i < len; i++) {
		vert_arr[i] = RM_edge_share_vert(edge_arr[i_prev], edge_arr[i]);
		if (vert_arr[i] == NULL) {
			return false;
		}
		i_prev = i;
	}
	return true;
}

bool RM_edges_from_verts(RMEdge **edge_arr, RMVert **vert_arr, const int len) {
	int i, i_prev = len - 1;
	for (i = 0; i < len; i++) {
		edge_arr[i_prev] = RM_edge_exists(vert_arr[i_prev], vert_arr[i]);
		if (edge_arr[i_prev] == NULL) {
			return false;
		}
		i_prev = i;
	}
	return true;
}

void RM_edges_from_verts_ensure(RMesh *mesh, RMEdge **edge_arr, RMVert **vert_arr, const int len) {
	int i, i_prev = len - 1;
	for (i = 0; i < len; i++) {
		edge_arr[i_prev] = RM_edge_create(mesh, vert_arr[i_prev], vert_arr[i], NULL, RM_CREATE_NO_DOUBLE);
		i_prev = i;
	}
}

static void rm_vert_attrs_copy(const RMesh *m_src, RMesh *m_dst, const RMVert *v_src, RMVert *v_dst, eCustomDataMask mask_exclude) {
	if ((m_src == m_dst) && (v_src == v_dst)) {
		ROSE_assert_msg(0, "RMVert: source and target match");
		return;
	}
	if ((mask_exclude & CD_MASK_NORMAL) == 0) {
		copy_v3_v3(v_dst->no, v_src->no);
	}
	CustomData_rmesh_free_block_data_exclude_by_type(&m_dst->vdata, v_dst->head.data, mask_exclude);
	CustomData_rmesh_copy_data_exclude_by_type(&m_src->vdata, &m_dst->vdata, v_src->head.data, &v_dst->head.data, mask_exclude);
}

static void rm_edge_attrs_copy(const RMesh *m_src, RMesh *m_dst, const RMEdge *e_src, RMEdge *e_dst, eCustomDataMask mask_exclude) {
	if ((m_src == m_dst) && (e_src == e_dst)) {
		ROSE_assert_msg(0, "RMEdge: source and target match");
		return;
	}
	CustomData_rmesh_free_block_data_exclude_by_type(&m_dst->edata, e_dst->head.data, mask_exclude);
	CustomData_rmesh_copy_data_exclude_by_type(&m_src->edata, &m_dst->edata, e_src->head.data, &e_dst->head.data, mask_exclude);
}

static void rm_loop_attrs_copy(const RMesh *m_src, RMesh *m_dst, const RMLoop *l_src, RMLoop *l_dst, eCustomDataMask mask_exclude) {
	if ((m_src == m_dst) && (l_src == l_dst)) {
		ROSE_assert_msg(0, "RMLoop: source and target match");
		return;
	}
	CustomData_rmesh_free_block_data_exclude_by_type(&m_dst->ldata, l_dst->head.data, mask_exclude);
	CustomData_rmesh_copy_data_exclude_by_type(&m_src->ldata, &m_dst->ldata, l_src->head.data, &l_dst->head.data, mask_exclude);
}

static void rm_face_attrs_copy(const RMesh *m_src, RMesh *m_dst, const RMFace *f_src, RMFace *f_dst, eCustomDataMask mask_exclude) {
	if ((m_src == m_dst) && (f_src == f_dst)) {
		ROSE_assert_msg(0, "RMFace: source and target match");
		return;
	}
	if ((mask_exclude & CD_MASK_NORMAL) == 0) {
		copy_v3_v3(f_dst->no, f_src->no);
	}
	CustomData_rmesh_free_block_data_exclude_by_type(&m_dst->pdata, f_dst->head.data, mask_exclude);
	CustomData_rmesh_copy_data_exclude_by_type(&m_src->pdata, &m_dst->pdata, f_src->head.data, &f_dst->head.data, mask_exclude);
	f_dst->mat_nr = f_src->mat_nr;
}

void RM_elem_attrs_copy_ex(const RMesh *mesh_src, RMesh *mesh_dst, const void *ele_src_v, void *ele_dst_v, const char hflag_mask, const uint64_t cd_mask_exclude) {
	const RMHeader *ele_src = ele_src_v;
	RMHeader *ele_dst = ele_dst_v;

	ROSE_assert(ele_src->htype == ele_dst->htype);
	ROSE_assert(ele_src != ele_dst);

	ele_dst->hflag = ((ele_dst->hflag & hflag_mask) | (ele_src->hflag & ~hflag_mask));

	switch (ele_dst->htype) {
		case RM_VERT: {
			rm_vert_attrs_copy(mesh_src, mesh_dst, (const RMVert *)ele_src, (RMVert *)ele_dst, cd_mask_exclude);
		} break;
		case RM_EDGE: {
			rm_edge_attrs_copy(mesh_src, mesh_dst, (const RMEdge *)ele_src, (RMEdge *)ele_dst, cd_mask_exclude);
		} break;
		case RM_LOOP: {
			rm_loop_attrs_copy(mesh_src, mesh_dst, (const RMLoop *)ele_src, (RMLoop *)ele_dst, cd_mask_exclude);
		} break;
		case RM_FACE: {
			rm_face_attrs_copy(mesh_src, mesh_dst, (const RMFace *)ele_src, (RMFace *)ele_dst, cd_mask_exclude);
		} break;
		default: {
			ROSE_assert_unreachable();
		} break;
	}
}

void RM_elem_attrs_copy(const RMesh *mesh_src, RMesh *mesh_dst, const void *ele_src, void *ele_dst) {
	/* TODO, default 'use_flags' to false */
	RM_elem_attrs_copy_ex(mesh_src, mesh_dst, ele_src, ele_dst, 0, 0);
}
