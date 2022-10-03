#include "gpu_backend.h"
#include "gpu_batch_private.h"
#include "gpu_vertex_buffer_private.h"
#include "gpu_index_buffer_private.h"
#include "gpu_vertex_format_private.h"
#include "gpu_shader_private.h"
#include "gpu_context_private.h"
#include "gpu_immediate_private.h"

#include "gpu/gpu_shader.h"
#include "gpu/gpu_batch.h"
#include "gpu/gpu_info.h"

#include "lib/lib_math_base.h"
#include "lib/lib_math_bits.h"
#include "lib/lib_math_vec.h"

#include <string.h>

using namespace rose::gpu;

GPU_Batch *GPU_batch_calloc ( ) {
	GPU_Batch *batch = GPU_Backend::Get ( )->BatchAlloc ( );
	memset ( batch , 0 , sizeof ( GPU_Batch ) );
	return batch;
}

GPU_Batch *GPU_batch_create_ex ( int prim_type , GPU_VertBuf *verts , GPU_IndexBuf *elem , unsigned int owns_flag ) {
        GPU_Batch *batch = GPU_batch_calloc ( );
        GPU_batch_init_ex ( batch , prim_type , verts , elem , owns_flag );
        return batch;
}

void GPU_batch_init_ex ( GPU_Batch *batch ,
                         int prim_type ,
                         GPU_VertBuf *verts ,
                         GPU_IndexBuf *elem ,
                         unsigned int owns_flag ) {
        assert ( verts != nullptr );
        /* Do not pass any other flag */
        assert ( ( owns_flag & ~( GPU_BATCH_OWNS_VBO | GPU_BATCH_OWNS_INDEX ) ) == 0 );

        batch->Verts [ 0 ] = verts;
        for ( int v = 1; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
                batch->Verts [ v ] = nullptr;
        }
        for ( auto &v : batch->Inst ) {
                v = nullptr;
        }
        batch->Elem = elem;
        batch->PrimType = prim_type;
        batch->Flag = owns_flag | GPU_BATCH_INIT | GPU_BATCH_DIRTY;
        batch->Shader = nullptr;
}

void GPU_batch_copy ( GPU_Batch *batch_dst , GPU_Batch *batch_src ) {
        GPU_batch_init_ex ( batch_dst , GPU_PRIM_POINTS , batch_src->Verts [ 0 ] , batch_src->Elem , GPU_BATCH_INVALID );

        batch_dst->PrimType = batch_src->PrimType;
        for ( int v = 1; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
                batch_dst->Verts [ v ] = batch_src->Verts [ v ];
        }
}

void GPU_batch_clear ( GPU_Batch *batch ) {
        if ( batch->Flag & GPU_BATCH_OWNS_INDEX ) {
                GPU_indexbuf_discard ( batch->Elem );
        }
        if ( batch->Flag & GPU_BATCH_OWNS_VBO_ANY ) {
                for ( int v = 0; ( v < GPU_BATCH_VBO_MAX_LEN ) && batch->Verts [ v ]; v++ ) {
                        if ( batch->Flag & ( GPU_BATCH_OWNS_VBO << v ) ) {
                                GPU_VERTBUF_DISCARD_SAFE ( batch->Verts [ v ] );
                        }
                }
        }
        if ( batch->Flag & GPU_BATCH_OWNS_INST_VBO_ANY ) {
                for ( int v = 0; ( v < GPU_BATCH_INST_VBO_MAX_LEN ) && batch->Inst [ v ]; v++ ) {
                        if ( batch->Flag & ( GPU_BATCH_OWNS_INST_VBO << v ) ) {
                                GPU_VERTBUF_DISCARD_SAFE ( batch->Inst [ v ] );
                        }
                }
        }
        batch->Flag = GPU_BATCH_INVALID;
}

void GPU_batch_discard ( GPU_Batch *batch ) {
        GPU_batch_clear ( batch );
        delete static_cast< Batch * >( batch );
}

void GPU_batch_instbuf_set ( GPU_Batch *batch , GPU_VertBuf *inst , bool own_vbo ) {
        assert ( inst );
        batch->Flag |= GPU_BATCH_DIRTY;

        if ( batch->Inst [ 0 ] && ( batch->Flag & GPU_BATCH_OWNS_INST_VBO ) ) {
                GPU_vertbuf_discard ( batch->Inst [ 0 ] );
        }
        batch->Inst [ 0 ] = inst;

        SET_FLAG_FROM_TEST ( batch->Flag , own_vbo , GPU_BATCH_OWNS_INST_VBO );
}

void GPU_batch_elembuf_set ( GPU_Batch *batch , GPU_IndexBuf *elem , bool own_ibo ) {
        assert ( elem );
        batch->Flag |= GPU_BATCH_DIRTY;

        if ( batch->Elem && ( batch->Flag & GPU_BATCH_OWNS_INDEX ) ) {
                GPU_indexbuf_discard ( batch->Elem );
        }
        batch->Elem = elem;

        SET_FLAG_FROM_TEST ( batch->Flag , own_ibo , GPU_BATCH_OWNS_INDEX );
}

int GPU_batch_instbuf_add_ex ( GPU_Batch *batch , GPU_VertBuf *insts , bool own_vbo ) {
        assert ( insts );
        batch->Flag |= GPU_BATCH_DIRTY;

        for ( unsigned int v = 0; v < GPU_BATCH_INST_VBO_MAX_LEN; v++ ) {
                if ( batch->Inst [ v ] == nullptr ) {
                        /* for now all VertexBuffers must have same vertex_len */
                        if ( batch->Inst [ 0 ] ) {
                                /* Allow for different size of vertex buffer (will choose the smallest number of verts). */
                                // BLI_assert(insts->vertex_len == batch->inst[0]->vertex_len);
                        }

                        batch->Inst [ v ] = insts;
                        SET_FLAG_FROM_TEST ( batch->Flag , own_vbo , ( unsigned int ) ( GPU_BATCH_OWNS_INST_VBO << v ) );
                        return v;
                }
        }
        /* we only make it this far if there is no room for another GPUVertBuf */
        fprintf ( stderr , "Not enough Instance VBO slot in batch\n" );
        return -1;
}

int GPU_batch_vertbuf_add_ex ( GPU_Batch *batch , GPU_VertBuf *verts , bool own_vbo ) {
        assert ( verts );
        batch->Flag |= GPU_BATCH_DIRTY;

        for ( unsigned int v = 0; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
                if ( batch->Verts [ v ] == nullptr ) {
                        /* for now all VertexBuffers must have same vertex_len */
                        if ( batch->Verts [ 0 ] != nullptr ) {
                                /* This is an issue for the HACK inside DRW_vbo_request(). */
                                // BLI_assert(verts->vertex_len == batch->verts[0]->vertex_len);
                        }
                        batch->Verts [ v ] = verts;
                        SET_FLAG_FROM_TEST ( batch->Flag , own_vbo , ( unsigned int ) ( GPU_BATCH_OWNS_VBO << v ) );
                        return v;
                }
        }
        /* we only make it this far if there is no room for another GPUVertBuf */
        fprintf ( stderr , "Not enough VBO slot in batch\n" );
        return -1;
}

bool GPU_batch_vertbuf_has ( GPU_Batch *batch , GPU_VertBuf *verts ) {
        for ( unsigned int v = 0; v < GPU_BATCH_VBO_MAX_LEN; v++ ) {
                if ( batch->Verts [ v ] == verts ) {
                        return true;
                }
        }
        return false;
}

void GPU_batch_resource_id_buf_set ( GPU_Batch *batch , GPU_StorageBuf *resource_id_buf ) {
        assert ( resource_id_buf );
        batch->Flag |= GPU_BATCH_DIRTY;
        batch->ResourceIDBuf = resource_id_buf;
}

void GPU_batch_set_shader ( GPU_Batch *batch , GPU_Shader *shader ) {
        batch->Shader = shader;
        GPU_shader_bind ( batch->Shader );
}

void GPU_batch_draw_parameter_get ( GPU_Batch *gpu_batch , int *r_v_count , int *r_v_first , int *r_base_index , int *r_i_count ) {
        Batch *batch = static_cast< Batch * >( gpu_batch );

        if ( batch->Elem ) {
                *r_v_count = batch->GetElem ( )->GetIndexLen ( );
                *r_v_first = batch->GetElem ( )->GetIndexStart ( );
                *r_base_index = batch->GetElem ( )->GetIndexBase ( );
        } else {
                *r_v_count = batch->GetVerts ( 0 )->mVertexLen;
                *r_v_first = 0;
                *r_base_index = -1;
        }

        int i_count = ( batch->Inst [ 0 ] ) ? batch->GetInst ( 0 )->mVertexLen : 1;
        /* Meh. This is to be able to use different numbers of verts in instance VBO's. */
        if ( batch->Inst [ 1 ] != nullptr ) {
                i_count = ROSE_MIN ( i_count , batch->GetInst ( 1 )->mVertexLen );
        }
        *r_i_count = i_count;
}

void GPU_batch_draw ( GPU_Batch *batch ) {
        GPU_shader_bind ( batch->Shader );
        GPU_batch_draw_advanced ( batch , 0 , 0 , 0 , 0 );
}

void GPU_batch_draw_range ( GPU_Batch *batch , int v_first , int v_count ) {
        GPU_shader_bind ( batch->Shader );
        GPU_batch_draw_advanced ( batch , v_first , v_count , 0 , 0 );
}

void GPU_batch_draw_instanced ( GPU_Batch *batch , int i_count ) {
        assert ( batch->Inst [ 0 ] == nullptr );

        GPU_shader_bind ( batch->Shader );
        GPU_batch_draw_advanced ( batch , 0 , 0 , 0 , i_count );
}

void GPU_batch_draw_advanced ( GPU_Batch *gpu_batch , int v_first , int v_count , int i_first , int i_count ) {
        assert ( Context::Get ( )->Shader != nullptr );
        Batch *batch = static_cast< Batch * >( gpu_batch );

        if ( v_count == 0 ) {
                if ( batch->Elem ) {
                        v_count = batch->GetElem ( )->GetIndexLen ( );
                } else {
                        v_count = batch->GetVerts ( 0 )->mVertexLen;
                }
        }
        if ( i_count == 0 ) {
                i_count = ( batch->Inst [ 0 ] ) ? batch->GetInst ( 0 )->mVertexLen : 1;
                /* Meh. This is to be able to use different numbers of verts in instance VBO's. */
                if ( batch->Inst [ 1 ] != nullptr ) {
                        i_count = ROSE_MIN ( i_count , batch->GetInst ( 1 )->mVertexLen );
                }
        }

        if ( v_count == 0 || i_count == 0 ) {
                /* Nothing to draw. */
                return;
        }

        batch->Draw ( v_first , v_count , i_first , i_count );
}

void GPU_batch_draw_indirect ( GPU_Batch *gpu_batch , GPU_StorageBuf *indirect_buf , intptr_t offset ) {
        assert ( Context::Get ( )->Shader != nullptr );
        assert ( indirect_buf != nullptr );
        Batch *batch = static_cast< Batch * >( gpu_batch );

        batch->DrawIndirect ( indirect_buf , offset );
}

void GPU_batch_multi_draw_indirect ( GPU_Batch *gpu_batch , GPU_StorageBuf *indirect_buf , int count , intptr_t offset , intptr_t stride ) {
        assert ( Context::Get ( )->Shader != nullptr );
        assert ( indirect_buf != nullptr );
        Batch *batch = static_cast< Batch * >( gpu_batch );

        batch->MultiDrawIndirect ( indirect_buf , count , offset , stride );
}

void GPU_batch_program_set_imm_shader ( GPU_Batch *batch ) {
        GPU_batch_set_shader ( batch , immGetShader ( ) );
}
