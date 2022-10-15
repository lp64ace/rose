#include "gl_vertex_array.h"
#include "gl_vertex_buffer.h"
#include "gl_storage_buffer.h"
#include "gl_index_buffer.h"
#include "gl_batch.h"
#include "gl_context.h"

#include <gl/glew.h>

namespace rose {
namespace gpu {

// Returns enabled vertex pointers as a bit-flag (one bit per attribute).
static uint16_t vbo_bind ( const ShaderInterface *interface , const GPU_VertFormat *format , unsigned int v_first , unsigned int v_len , const bool use_instancing ) {
        uint16_t enabled_attrib = 0;
        const unsigned int attr_len = format->AttrLen;
        unsigned int stride = format->Stride;
        unsigned int offset = 0;
        unsigned int divisor = ( use_instancing ) ? 1 : 0;

        for ( unsigned int a_idx = 0; a_idx < attr_len; a_idx++ ) {
                const GPU_VertAttr *a = &format->Attrs [ a_idx ];

                if ( format->Deinterleaved ) {
                        offset += ( ( a_idx == 0 ) ? 0 : format->Attrs [ a_idx - 1 ].Size ) * v_len;
                        stride = a->Size;
                } else {
                        offset = a->Offset;
                }

                /* This is in fact an offset in memory. */
                const void *pointer = ( const unsigned char * ) ( intptr_t ) ( offset + v_first * stride );
                const unsigned int type = comp_type_to_gl ( a->CompType );

                for ( unsigned int n_idx = 0; n_idx < a->NameLen; n_idx++ ) {
                        const char *name = GPU_vertformat_attr_name_get ( format , a , n_idx );
                        const ShaderInput *input = interface->AttrGet ( name );

                        if ( input == nullptr || input->Location == -1 ) {
                                continue;
                        }

                        enabled_attrib |= ( 1 << input->Location );

                        if ( ELEM ( a->CompLen , 16 , 12 , 8 ) ) {
                                assert ( a->FetchMode == GPU_FETCH_FLOAT );
                                assert ( a->CompType == GPU_COMP_F32 );
                                for ( int i = 0; i < a->CompLen / 4; i++ ) {
                                        glEnableVertexAttribArray ( input->Location + i );
                                        glVertexAttribDivisor ( input->Location + i , divisor );
                                        glVertexAttribPointer ( input->Location + i , 4 , type , GL_FALSE , stride , ( const unsigned char * ) pointer + i * 16 );
                                }
                        } else {
                                glEnableVertexAttribArray ( input->Location );
                                glVertexAttribDivisor ( input->Location , divisor );

                                switch ( a->FetchMode ) {
                                        case GPU_FETCH_FLOAT:
                                        case GPU_FETCH_INT_TO_FLOAT: {
                                                glVertexAttribPointer ( input->Location , a->CompLen , type , GL_FALSE , stride , pointer );
                                        }break;
                                        case GPU_FETCH_INT_TO_FLOAT_UNIT: {
                                                glVertexAttribPointer ( input->Location , a->CompLen , type , GL_TRUE , stride , pointer );
                                        }break;
                                        case GPU_FETCH_INT: {
                                                glVertexAttribIPointer ( input->Location , a->CompLen , type , stride , pointer );
                                        }break;
                                }
                        }
                }
        }
        return enabled_attrib;
}

void GLVertArray::UpdateBindings ( const unsigned int vao , const GPU_Batch *batch_ , const ShaderInterface *interface , const int base_instance ) {
        const GLBatch *batch = static_cast< const GLBatch * >( batch_ );
        uint16_t attr_mask = interface->mEnabledAttrMask;

        glBindVertexArray ( vao );

        /* Reverse order so first VBO'S have more prevalence (in term of attribute override). */
        for ( int v = GPU_BATCH_VBO_MAX_LEN - 1; v > -1; v-- ) {
                GLVertBuf *vbo = batch->GetVerts ( v );
                if ( vbo ) {
                        vbo->Bind ( );
                        attr_mask &= ~vbo_bind ( interface , &vbo->mFormat , 0 , vbo->mVertexLen , false );
                }
        }

        for ( int v = GPU_BATCH_INST_VBO_MAX_LEN - 1; v > -1; v-- ) {
                GLVertBuf *vbo = batch->GetInst ( v );
                if ( vbo ) {
                        vbo->Bind ( );
                        attr_mask &= ~vbo_bind ( interface , &vbo->mFormat , base_instance , vbo->mVertexLen , true );
                }
        }

        if ( batch->ResourceIDBuf ) {
                const ShaderInput *input = interface->AttrGet ( "drw_ResourceID" );
                if ( input ) {
                        dynamic_cast< GLStorageBuf * >( unwrap ( batch->ResourceIDBuf ) )->BindAs ( GL_ARRAY_BUFFER );
                        glEnableVertexAttribArray ( input->Location );
                        glVertexAttribDivisor ( input->Location , 1 );
                        glVertexAttribIPointer ( input->Location , 1 , GL_INT , sizeof ( uint32_t ) , ( GLvoid * ) nullptr );
                        attr_mask &= ~( 1 << input->Location );
                }
        }

        if ( attr_mask != 0 && GLContext::VertexAttribBindingSupport ) {
                for ( uint16_t mask = 1 , a = 0; a < 16; a++ , mask <<= 1 ) {
                        if ( attr_mask & mask ) {
                                GLContext *ctx = GLContext::Get ( );
                                /* This replaces glVertexAttrib4f(a, 0.0f, 0.0f, 0.0f, 1.0f); with a more modern style.
                                 * Fix issues for some drivers (see T75069). */
                                glBindVertexBuffer ( a , ctx->mDefaultAttrVbo , ( intptr_t ) 0 , ( intptr_t ) 0 );
                                glEnableVertexAttribArray ( a );
                                glVertexAttribFormat ( a , 4 , GL_FLOAT , GL_FALSE , 0 );
                                glVertexAttribBinding ( a , a );
                        }
                }
        }

        if ( batch->Elem ) {
                /* Binds the index buffer. This state is also saved in the VAO. */
                static_cast< GLIndexBuf * >( unwrap ( batch->Elem ) )->Bind ( );
        }
}

void GLVertArray::UpdateBindings ( const unsigned int vao , unsigned int v_first , const GPU_VertFormat *format , const ShaderInterface *interface ) {
        glBindVertexArray ( vao );

        vbo_bind ( interface , format , v_first , 0 , false );
}

}
}
