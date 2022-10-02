#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_WRITE_NONE				0x00000000
#define GPU_WRITE_RED				0x00000001
#define GPU_WRITE_GREEN				0x00000002
#define GPU_WRITE_BLUE				0x00000004
#define GPU_WRITE_ALPHA				0x00000008
#define GPU_WRITE_DEPTH				0x00000010
#define GPU_WRITE_STENCIL			0x00000020
#define GPU_WRITE_COLOR				(GPU_WRITE_RED|GPU_WRITE_GREEN|GPU_WRITE_BLUE|GPU_WRITE_ALPHA)

#define GPU_BARRIER_NONE			0x00000000
#define GPU_BARRIER_COMMAND			0x00000001
#define GPU_BARRIER_FRAMEBUFFER			0x00000002
#define GPU_BARRIER_SHADER_IMAGE_ACCESS		0x00000004
#define GPU_BARRIER_SHADER_STORAGE		0x00000008
#define GPU_BARRIER_TEXTURE_FETCH		0x00000010
#define GPU_BARRIER_TEXTURE_UPDATE		0x00000020
#define GPU_BARRIER_VERTEX_ATTRIB_ARRAY		0x00000040
#define GPU_BARRIER_ELEMENT_ARRAY		0x00000080

#define GPU_BARRIER_STAGE_VERTEX		0x00000001
#define GPU_BARRIER_STAGE_FRAGMENT		0x00000002
#define GPU_BARRIER_STAGE_COMPUTE		0x00000004
#define GPU_BARRIER_STAGE_ANY_GRAPHICS		(GPU_BARRIER_STAGE_VERTEX|GPU_BARRIER_STAGE_FRAGMENT)
#define GPU_BARRIER_STAGE_ANY			(GPU_BARRIER_STAGE_VERTEX|GPU_BARRIER_STAGE_FRAGMENT|GPU_BARRIER_STAGE_COMPUTE)

#define GPU_BLEND_NONE				0x00000000
#define GPU_BLEND_ALPHA				0x00000001
#define GPU_BLEND_ALPHA_PREMULT			0x00000002
#define GPU_BLEND_ADDITIVE			0x00000003
#define GPU_BLEND_ADDITIVE_PREMULT		0x00000004
#define GPU_BLEND_MULTIPLY			0x00000005
#define GPU_BLEND_ALPHA_PREMULT			0x00000006
#define GPU_BLEND_SUBTRACT			0x00000007
#define GPU_BLEND_INVERT			0x00000008
#define GPU_BLEND_OIT				0x00000009
#define GPU_BLEND_BACKGROUND			0x0000000A
#define GPU_BLEND_CUSTOM			0x0000000B
#define GPU_BLEND_ALPHA_UNDER_PREMUL		0x0000000C

#define GPU_DEPTH_NONE				0x00000000
#define GPU_DEPTH_ALWAYS			0x00000001
#define GPU_DEPTH_LESS				0x00000002
#define GPU_DEPTH_LESS_EQUAL			0x00000003
#define GPU_DEPTH_EQUAL				0x00000004
#define GPU_DEPTH_GREATER			0x00000005
#define GPU_DEPTH_GREATER_EQUAL			0x00000006

#define GPU_STENCIL_NONE			0x00000000
#define GPU_STENCIL_ALWAYS			0x00000001
#define GPU_STENCIL_EQUAL			0x00000002
#define GPU_STENCIL_NEQUAL			0x00000003

#define GPU_STENCIL_OP_NONE			0x00000000
#define GPU_STENCIL_OP_REPLACE			0x00000001
#define GPU_STENCIL_OP_COUNT_DEPTH_PASS		0x00000002
#define GPU_STENCIL_OP_COUNT_DEPTH_FAIL		0x00000003

#define GPU_CULL_NONE				0x00000000
#define GPU_CULL_FRONT				0x00000001
#define GPU_CULL_BACK				0x00000002

#define GPU_VERTEX_LAST				0x00000000
#define GPU_VERTEX_FIRST			0x00000001

        void GPU_blend ( unsigned int blend );
        void GPU_face_culling ( unsigned int culling );
        void GPU_depth_test ( unsigned int test );
        void GPU_stencil_test ( unsigned int test );
        void GPU_provoking_vertex ( unsigned int vert );
        void GPU_front_facing ( bool invert );
        void GPU_depth_range ( float near , float far );
        void GPU_scissor_test ( bool enable );
        void GPU_line_smooth ( bool enable );
        /**
         * \note By convention, this is set as needed and not reset back to 1.0.
         * This means code that draws lines must always set the line width beforehand,
         * but is not expected to restore it's previous value.
         */
        void GPU_line_width ( float width );
        void GPU_logic_op_xor_set ( bool enable );
        void GPU_point_size ( float size );
        void GPU_polygon_smooth ( bool enable );
        /**
         * Programmable point size:
         * - Shaders set their own point size when enabled
         * - Use GPU_point_size when disabled.
         *
         * TODO: remove and use program point size everywhere.
         */
        void GPU_program_point_size ( bool enable );
        void GPU_scissor ( int x , int y , int width , int height );
        void GPU_scissor_get ( int coords [ 4 ] );
        void GPU_viewport ( int x , int y , int width , int height );
        void GPU_viewport_size_get_f ( float coords [ 4 ] );
        void GPU_viewport_size_get_i ( int coords [ 4 ] );
        void GPU_write_mask ( unsigned int mask );
        void GPU_color_mask ( bool r , bool g , bool b , bool a );
        void GPU_depth_mask ( bool depth );
        bool GPU_depth_mask_get ( void );
        void GPU_shadow_offset ( bool enable );
        void GPU_clip_distances ( int distances_enabled );
        bool GPU_mipmap_enabled ( void );
        void GPU_state_set ( unsigned int write_mask ,
                             unsigned int blend ,
                             unsigned int culling_test ,
                             unsigned int depth_test ,
                             unsigned int stencil_test ,
                             unsigned int stencil_op ,
                             unsigned int provoking_vert );

        void GPU_stencil_reference_set ( unsigned int reference );
        void GPU_stencil_write_mask_set ( unsigned int write_mask );
        void GPU_stencil_compare_mask_set ( unsigned int compare_mask );

        unsigned int GPU_face_culling_get ( void );
        unsigned int GPU_blend_get ( void );
        unsigned int GPU_depth_test_get ( void );
        unsigned int GPU_write_mask_get ( void );
        unsigned int GPU_stencil_mask_get ( void );
        unsigned int GPU_stencil_test_get ( void );

        float GPU_line_width_get ( void );

        void GPU_flush ( void );
        void GPU_finish ( void );
        void GPU_apply_state ( void );

        void GPU_memory_barrier ( unsigned int barrier );

#ifdef __cplusplus
}
#endif
