#pragma once

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_PRIM_POINTS			0x00000001
#define GPU_PRIM_LINES			0x00000002
#define GPU_PRIM_TRIS			0x00000003
#define GPU_PRIM_LINE_STRIP		0x00000004
#define GPU_PRIM_LINE_LOOP		0x00000005
#define GPU_PRIM_TRI_STRIP		0x00000006
#define GPU_PRIM_TRI_FAN		0x00000007
#define GPU_PRIM_LINES_ADJ		0x00000008
#define GPU_PRIM_TRIS_ADJ		0x00000009
#define GPU_PRIM_LINE_STRIP_ADJ		0x0000000A
#define GPU_PRIM_NONE			0x0000000B

	// what types of primitives does each shader expect?

#define GPU_PRIM_CLASS_NONE		0x00000000
#define GPU_PRIM_CLASS_POINT		0x00000001
#define GPU_PRIM_CLASS_LINE		0x00000002
#define GPU_PRIM_CLASS_SURFACE		0x00000004
#define GPU_PRIM_CLASS_ANY		0x00000007

	inline int gpu_get_prim_count_from_type ( unsigned int vertex_len , unsigned int prim_type ) {
		/* does vertex_len make sense for this primitive type? */
		if ( vertex_len == 0 ) {
			return 0;
		}

		switch ( prim_type ) {
			case GPU_PRIM_POINTS: {
				return vertex_len;
			}break;
			case GPU_PRIM_LINES: {
				assert ( vertex_len % 2 == 0 );
				return vertex_len / 2;
			}break;
			case GPU_PRIM_LINE_STRIP: {
				return vertex_len - 1;
			}break;
			case GPU_PRIM_LINE_LOOP: {
				return vertex_len;
			}break;
			case GPU_PRIM_LINES_ADJ: {
				assert ( vertex_len % 4 == 0 );
				return vertex_len / 4;
			}break;
			case GPU_PRIM_LINE_STRIP_ADJ: {
				return vertex_len - 2;
			}break;
			case GPU_PRIM_TRIS: {
				assert ( vertex_len % 3 == 0 );
				return vertex_len / 3;
			}break;
			case GPU_PRIM_TRI_STRIP: {
				assert ( vertex_len >= 3 );
				return vertex_len - 2;
			}break;
			case GPU_PRIM_TRI_FAN: {
				assert ( vertex_len >= 3 );
				return vertex_len - 2;
			}break;
			case GPU_PRIM_TRIS_ADJ: {
				assert ( vertex_len % 6 == 0 );
				return vertex_len / 6;
			}break;
			default: {
				fprintf ( stderr , "Unknown primitive type.\n" );
				return 0;
			}break;
		}
	}

	inline bool is_restart_compatible ( unsigned int type ) {
		switch ( type ) {
			case GPU_PRIM_POINTS:
			case GPU_PRIM_LINES:
			case GPU_PRIM_TRIS:
			case GPU_PRIM_LINES_ADJ:
			case GPU_PRIM_TRIS_ADJ:
			case GPU_PRIM_NONE:
			default: {
				return false;
			}
			case GPU_PRIM_LINE_STRIP:
			case GPU_PRIM_LINE_LOOP:
			case GPU_PRIM_TRI_STRIP:
			case GPU_PRIM_TRI_FAN:
			case GPU_PRIM_LINE_STRIP_ADJ: {
				return true;
			}
		}
		return false;
	}

#ifdef __cplusplus
}
#endif
