#include "MEM_guardedalloc.h"

#include "DNA_customdata_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "LIB_alloca.h"
#include "LIB_array.hh"
#include "LIB_index_range.hh"
#include "LIB_listbase.h"
#include "LIB_math_matrix_types.hh"
#include "LIB_math_vector_types.hh"
#include "LIB_math_vector.h"
#include "LIB_span.hh"
#include "LIB_string_ref.hh"
#include "LIB_task.hh"
#include "LIB_vector.hh"

#include "KER_customdata.h"
#include "KER_mesh.h"

#include "RM_include.h"

#include "rm_mesh_convert.h"
#include "rm_private.h"

ROSE_STATIC void assert_rmesh_has_no_mesh_only_attributes(const RMesh *rm) {
	(void)rm;
	ROSE_assert(!CustomData_has_layer_named(&rm->vdata, CD_PROP_FLOAT3, "position"));
	ROSE_assert(!CustomData_has_layer_named(&rm->ldata, CD_PROP_INT32, ".corner_vert"));
	ROSE_assert(!CustomData_has_layer_named(&rm->ldata, CD_PROP_INT32, ".corner_edge"));
	/* The "hide" attributes are stored as flags on #RMesh. */
	ROSE_assert(!CustomData_has_layer_named(&rm->vdata, CD_PROP_BOOL, ".hide_vert"));
	ROSE_assert(!CustomData_has_layer_named(&rm->edata, CD_PROP_BOOL, ".hide_edge"));
	ROSE_assert(!CustomData_has_layer_named(&rm->pdata, CD_PROP_BOOL, ".hide_poly"));
	/* The "selection" attributes are stored as flags on #RMesh. */
	ROSE_assert(!CustomData_has_layer_named(&rm->vdata, CD_PROP_BOOL, ".select_vert"));
	ROSE_assert(!CustomData_has_layer_named(&rm->edata, CD_PROP_BOOL, ".select_edge"));
	ROSE_assert(!CustomData_has_layer_named(&rm->pdata, CD_PROP_BOOL, ".select_poly"));
}

ROSE_STATIC void rm_vert_table_build(RMesh *rm, rose::Array<RMVert *> &vtable) {
	RMIter iter;
	int index;
	
	RMVert *v;
	RM_ITER_MESH_INDEX (v, &iter, rm, RM_VERTS_OF_MESH, index) {
		RM_elem_index_set(v, index);
		vtable[index] = v;
	}
}

ROSE_STATIC void rm_edge_table_build(RMesh *rm, rose::Array<RMEdge *> &etable) {
	RMIter iter;
	int index;
	
	RMEdge *e;
	RM_ITER_MESH_INDEX (e, &iter, rm, RM_EDGES_OF_MESH, index) {
		RM_elem_index_set(e, index);
		etable[index] = e;
	}
}

ROSE_STATIC void rm_poly_loop_table_build(RMesh *rm, rose::Array<RMFace *> &ftable, rose::Array<RMLoop *> &ltable) {
	RMIter iter;
	
	int f_index = 0;
	int l_index = 0;
	
	RMFace *f;
	RM_ITER_MESH_INDEX (f, &iter, rm, RM_FACES_OF_MESH, f_index) {
		RM_elem_index_set(f, f_index);
		ftable[f_index] = f;
		
		RMLoop *l = RM_FACE_FIRST_LOOP(f);
		for ([[maybe_unused]] const int i : rose::IndexRange(f->len)) {
			RM_elem_index_set(l, l_index);
			ltable[l_index] = l;
			l_index++;
			l = l->next;
		}
	}
}

ROSE_STATIC void rm_to_mesh_verts(const RMesh *rm, const rose::Array<RMVert *>& rm_verts, Mesh *mesh) {
	CustomData_free_layer_named(&mesh->vdata, "position", mesh->totvert);
	CustomData_add_layer_named(&mesh->vdata, CD_PROP_FLOAT3, CD_CONSTRUCT, mesh->totvert, "position");
	
	rose::MutableSpan<float3> dst_vert_positions = rose::MutableSpan<float3>{
		reinterpret_cast<float3 *>(KER_mesh_vert_positions_for_write(mesh)),
		(size_t)mesh->totvert
	};
	
	rose::threading::parallel_for(dst_vert_positions.index_range(), 1024, [&](const rose::IndexRange range) {
		for (const int index : range) {
			const RMVert *src_vert = rm_verts[index];
			
			copy_v3_v3(dst_vert_positions[index], src_vert->co);
		}
	});
}

ROSE_STATIC void rm_to_mesh_edges(const RMesh *rm, const rose::Array<RMEdge *>& rm_edges, Mesh *mesh) {
	CustomData_free_layer_named(&mesh->edata, ".edge_verts", mesh->totedge);
	CustomData_add_layer_named(&mesh->edata, CD_PROP_INT32_2D, CD_CONSTRUCT, mesh->totedge, ".edge_verts");
	
	rose::MutableSpan<int2> dst_edges = rose::MutableSpan<int2>{
		reinterpret_cast<int2 *>(KER_mesh_edges_for_write(mesh)),
		(size_t)mesh->totedge
	};
	
	rose::threading::parallel_for(dst_edges.index_range(), 1024, [&](const rose::IndexRange range) {
		for (const int index : range) {
			const RMEdge *src_edge = rm_edges[index];
			
			dst_edges[index] = int2(
				RM_elem_index_get(src_edge->v1),
				RM_elem_index_get(src_edge->v2)
			);
		}
	});
}

ROSE_STATIC void rm_to_mesh_polys(const RMesh *rm, const rose::Array<RMFace *>& rm_polys, Mesh *mesh) {
	KER_mesh_poly_offsets_ensure_alloc(mesh);
	
	rose::MutableSpan<int> dst_poly_offsets = rose::MutableSpan<int>{
		KER_mesh_poly_offsets_for_write(mesh),
		(size_t)mesh->totpoly + 1,
	};
	
	/**
	 * The last offset is already set by #KER_mesh_poly_offsets_ensure_alloc, handy!
	 * The reason we use #rm_polys.index_range() here is because #mesh->totpoly + 1 will lead to an out-of-bounds.
	 * As said above #dst_poly_offsets's last index will already be initialized by #KER_mesh_poly_offsets_ensure_alloc.
	 */
	rose::threading::parallel_for(rm_polys.index_range(), 1024, [&](const rose::IndexRange range) {
		for (const int index : range) {
			const RMFace *src_poly = rm_polys[index];
			
			/** We store the index of the first loop of the face. */
			dst_poly_offsets[index] = RM_elem_index_get(RM_FACE_FIRST_LOOP(src_poly));
		}
	});
}

ROSE_STATIC void rm_to_mesh_loops(const RMesh *rm, const rose::Array<RMLoop *>& rm_loops, Mesh *mesh) {
	CustomData_free_layer_named(&mesh->ldata, ".corner_vert", mesh->totloop);
	CustomData_free_layer_named(&mesh->ldata, ".corner_edge", mesh->totloop);
	CustomData_add_layer_named(&mesh->ldata, CD_PROP_INT32, CD_CONSTRUCT, mesh->totloop, ".corner_vert");
	CustomData_add_layer_named(&mesh->ldata, CD_PROP_INT32, CD_CONSTRUCT, mesh->totloop, ".corner_edge");
	
	rose::MutableSpan<int> dst_corner_verts = rose::MutableSpan<int>{
		KER_mesh_corner_verts_for_write(mesh),
		(size_t)mesh->totloop
	};
	rose::MutableSpan<int> dst_corner_edges = rose::MutableSpan<int>{
		KER_mesh_corner_edges_for_write(mesh),
		(size_t)mesh->totloop
	};
	
	rose::threading::parallel_for(dst_corner_verts.index_range(), 1024, [&](const rose::IndexRange range) {
		for (const int index : range) {
			const RMLoop *src_loop = rm_loops[index];
			
			dst_corner_verts[index] = RM_elem_index_get(src_loop->v);
			dst_corner_edges[index] = RM_elem_index_get(src_loop->e);
		}
	});
}

void RM_mesh_rm_to_me(Main *main, RMesh *rm, Mesh *me, const struct RMeshToMeshParams *params) {
	KER_mesh_geometry_clear(me);
	
	/* Add new custom data. */
	me->totvert = rm->totvert;
	me->totedge = rm->totedge;
	me->totloop = rm->totloop;
	me->totpoly = rm->totface;
	
	/**
	 * Will be overwritten with a valid value if 'dotess' is set, otherwise we 
	 * end up with 'me->totface' and me->mface == nullptr which can crash T28625.
	 */
	me->totface = 0;
	
	assert_rmesh_has_no_mesh_only_attributes(rm);
	{
		CustomData_MeshMasks mask = CD_MASK_MESH;
		CustomData_MeshMasks_update(&mask, &params->cd_mask_extra);
		CustomData_copy(&rm->vdata, &me->vdata, mask.vmask, me->totvert);
		CustomData_copy(&rm->edata, &me->edata, mask.emask, me->totedge);
		CustomData_copy(&rm->ldata, &me->ldata, mask.lmask, me->totloop);
		CustomData_copy(&rm->pdata, &me->pdata, mask.pmask, me->totpoly);
	}
	
	rose::Array<RMVert *> vert_table;
	rose::Array<RMEdge *> edge_table;
	rose::Array<RMLoop *> loop_table;
	rose::Array<RMFace *> poly_table;
	
	rose::threading::parallel_invoke((me->totpoly + me->totedge) > 1024,
		[&]() {
			vert_table.reinitialize(rm->totvert);
			rm_vert_table_build(rm, vert_table);
		},
		[&]() {
			edge_table.reinitialize(rm->totedge);
			rm_edge_table_build(rm, edge_table);
		},
		[&]() {
			poly_table.reinitialize(rm->totface);
			loop_table.reinitialize(rm->totloop);
			rm_poly_loop_table_build(rm, poly_table, loop_table);
		}
	);
	
	rm->elem_index_dirty &= ~(RM_VERT | RM_EDGE | RM_FACE | RM_LOOP);
	
	/** TODO(@lp64ace) Also copy selection and hide properties from each element header when needed. */
	rose::threading::parallel_invoke((me->totpoly + me->totedge) > 1024,
		[&]() {
			rm_to_mesh_verts(rm, vert_table, me);
		},
		[&]() {
			rm_to_mesh_edges(rm, edge_table, me);
		},
		[&]() {
			rm_to_mesh_polys(rm, poly_table, me);
		},
		[&]() {
			rm_to_mesh_loops(rm, loop_table, me);
		}
	);
}
