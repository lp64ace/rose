#include "gpu_state_private.h"
#include "gpu_context_private.h"
#include "gpu_framebuffer_private.h"

#include "gpu/gpu_state.h"

using namespace rose::gpu;

#define SET_STATE(_prefix, _state, _value) \
  do { \
    StateManager *stack = Context::Get()->StateManager; \
    auto &state_object = stack->_prefix##State; \
    state_object._state = (_value); \
  } while (0)

#define SET_IMMUTABLE_STATE(_state, _value) SET_STATE(, _state, _value)
#define SET_MUTABLE_STATE(_state, _value) SET_STATE(Mutable, _state, _value)

void GPU_blend ( unsigned int blend ) {
	SET_IMMUTABLE_STATE ( Blend , blend );
}

void GPU_face_culling ( unsigned int culling ) {
        SET_IMMUTABLE_STATE ( CullingTest , culling );
}

unsigned int GPU_face_culling_get ( ) {
        GPU_State &state = Context::Get ( )->StateManager->State;
        return ( unsigned int ) state.CullingTest;
}

void GPU_front_facing ( bool invert ) {
        SET_IMMUTABLE_STATE ( InvertFacing , invert );
}

void GPU_provoking_vertex ( unsigned int vert ) {
        SET_IMMUTABLE_STATE ( ProvokingVertex , vert );
}

void GPU_depth_test ( unsigned int test ) {
        SET_IMMUTABLE_STATE ( DepthTest , test );
}

void GPU_stencil_test ( unsigned int test ) {
        SET_IMMUTABLE_STATE ( StencilTest , test );
}

void GPU_line_smooth ( bool enable ) {
        SET_IMMUTABLE_STATE ( LineSmooth , enable );
}

void GPU_polygon_smooth ( bool enable ) {
        SET_IMMUTABLE_STATE ( PolygonSmooth , enable );
}

void GPU_logic_op_xor_set ( bool enable ) {
        SET_IMMUTABLE_STATE ( LogicOpXor , enable );
}

void GPU_write_mask ( unsigned int mask ) {
        SET_IMMUTABLE_STATE ( WriteMask , mask );
}

void GPU_color_mask ( bool r , bool g , bool b , bool a ) {
        StateManager *stack = Context::Get ( )->StateManager;
        auto &state = stack->State;
        uint32_t write_mask = state.WriteMask;
        SET_FLAG_FROM_TEST ( write_mask , r , ( uint32_t ) GPU_WRITE_RED );
        SET_FLAG_FROM_TEST ( write_mask , g , ( uint32_t ) GPU_WRITE_GREEN );
        SET_FLAG_FROM_TEST ( write_mask , b , ( uint32_t ) GPU_WRITE_BLUE );
        SET_FLAG_FROM_TEST ( write_mask , a , ( uint32_t ) GPU_WRITE_ALPHA );
        state.WriteMask = write_mask;
}

void GPU_depth_mask ( bool depth ) {
        StateManager *stack = Context::Get ( )->StateManager;
        auto &state = stack->State;
        uint32_t write_mask = state.WriteMask;
        SET_FLAG_FROM_TEST ( write_mask , depth , ( uint32_t ) GPU_WRITE_DEPTH );
        state.WriteMask = write_mask;
}

void GPU_shadow_offset ( bool enable ) {
        SET_IMMUTABLE_STATE ( ShadowBias , enable );
}

void GPU_clip_distances ( int distances_enabled ) {
        SET_IMMUTABLE_STATE ( ClipDistances , distances_enabled );
}

void GPU_state_set ( unsigned int write_mask ,
                     unsigned int blend ,
                     unsigned int culling_test ,
                     unsigned int depth_test ,
                     unsigned int stencil_test ,
                     unsigned int stencil_op ,
                     unsigned int provoking_vert ) {
        StateManager *stack = Context::Get ( )->StateManager;
        auto &state = stack->State;
        state.WriteMask = ( uint32_t ) write_mask;
        state.Blend = ( uint32_t ) blend;
        state.CullingTest = ( uint32_t ) culling_test;
        state.DepthTest = ( uint32_t ) depth_test;
        state.StencilTest = ( uint32_t ) stencil_test;
        state.StencilOp = ( uint32_t ) stencil_op;
        state.ProvokingVertex = ( uint32_t ) provoking_vert;
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name Mutable State Setters
 * \{ */

void GPU_depth_range ( float near , float far ) {
        StateManager *stack = Context::Get ( )->StateManager;
        auto &state = stack->MutableState;
        state.DepthRange [ 0 ] = near;
        state.DepthRange [ 1 ] = far;
}

void GPU_line_width ( float width ) {
        width = ROSE_MAX ( 1.0f , width );
        SET_MUTABLE_STATE ( LineWidth , width );
}

void GPU_point_size ( float size ) {
        StateManager *stack = Context::Get ( )->StateManager;
        auto &state = stack->MutableState;
        /* Keep the sign of point_size since it represents the enable state. */
        state.PointSize = size * ( ( state.PointSize > 0.0 ) ? 1.0f : -1.0f );
}

void GPU_program_point_size ( bool enable ) {
        StateManager *stack = Context::Get ( )->StateManager;
        auto &state = stack->MutableState;
        /* Set point size sign negative to disable. */
        state.PointSize = fabsf ( state.PointSize ) * ( enable ? 1 : -1 );
}

void GPU_scissor_test ( bool enable ) {
        Context::Get ( )->ActiveFb->ScissorTestSet ( enable );
}

void GPU_scissor ( int x , int y , int width , int height ) {
        int scissor_rect [ 4 ] = { x, y, width, height };
        Context::Get ( )->ActiveFb->SetScissor ( scissor_rect );
}

void GPU_viewport ( int x , int y , int width , int height ) {
        int viewport_rect [ 4 ] = { x, y, width, height };
        Context::Get ( )->ActiveFb->SetViewport ( viewport_rect );
}

void GPU_stencil_reference_set ( unsigned int reference ) {
        SET_MUTABLE_STATE ( StencilReference , ( uint8_t ) reference );
}

void GPU_stencil_write_mask_set ( unsigned int write_mask ) {
        SET_MUTABLE_STATE ( StencilWriteMask , ( uint8_t ) write_mask );
}

void GPU_stencil_compare_mask_set ( unsigned int compare_mask ) {
        SET_MUTABLE_STATE ( StencilCompareMask , ( uint8_t ) compare_mask );
}

/** \} */

/* -------------------------------------------------------------------- */
/** \name State Getters
 * \{ */

unsigned int GPU_blend_get ( ) {
        GPU_State &state = Context::Get ( )->StateManager->State;
        return ( unsigned int ) state.Blend;
}

unsigned int GPU_write_mask_get ( ) {
        GPU_State &state = Context::Get ( )->StateManager->State;
        return ( unsigned int ) state.WriteMask;
}

unsigned int GPU_stencil_mask_get ( ) {
        const GPU_StateMutable &state = Context::Get ( )->StateManager->MutableState;
        return state.StencilWriteMask;
}

unsigned int GPU_depth_test_get ( ) {
        GPU_State &state = Context::Get ( )->StateManager->State;
        return ( unsigned int ) state.DepthTest;
}

unsigned int GPU_stencil_test_get ( ) {
        GPU_State &state = Context::Get ( )->StateManager->State;
        return ( unsigned int ) state.StencilTest;
}

float GPU_line_width_get ( ) {
        const GPU_StateMutable &state = Context::Get ( )->StateManager->MutableState;
        return state.LineWidth;
}

void GPU_scissor_get ( int coords [ 4 ] ) {
        Context::Get ( )->ActiveFb->GetScissor ( coords );
}

void GPU_viewport_size_get_f ( float coords [ 4 ] ) {
        int viewport [ 4 ];
        Context::Get ( )->ActiveFb->GetViewport ( viewport );
        for ( int i = 0; i < 4; i++ ) {
                coords [ i ] = viewport [ i ];
        }
}

void GPU_viewport_size_get_i ( int coords [ 4 ] ) {
        Context::Get ( )->ActiveFb->GetViewport ( coords );
}

bool GPU_depth_mask_get ( ) {
        const GPU_State &state = Context::Get ( )->StateManager->State;
        return ( state.WriteMask & GPU_WRITE_DEPTH ) != 0;
}

bool GPU_mipmap_enabled ( ) {
        /* TODO(fclem): this used to be a userdef option. */
        return true;
}
