#pragma once

#include <stdint.h>

#define GHOST_DECLARE_HANDLE(name) typedef struct name##__ { int unused; } *name;

GHOST_DECLARE_HANDLE ( GHOST_SystemHandle );
GHOST_DECLARE_HANDLE ( GHOST_TimerTaskHandle );
GHOST_DECLARE_HANDLE ( GHOST_WindowHandle );
GHOST_DECLARE_HANDLE ( GHOST_EventHandle );
GHOST_DECLARE_HANDLE ( GHOST_ContextHandle );
GHOST_DECLARE_HANDLE ( GHOST_EventConsumerHandle );

typedef enum {
	GHOST_kButtonMaskNone ,
	GHOST_kButtonMaskLeft ,
	GHOST_kButtonMaskMiddle ,
	GHOST_kButtonMaskRight ,
	GHOST_kButtonMaskButton4 ,
	GHOST_kButtonMaskButton5 ,
	/* Trackballs and programmable buttons. */
	GHOST_kButtonMaskButton6 ,
	GHOST_kButtonMaskButton7 ,
	GHOST_kButtonNum
} GHOST_TButton;

typedef enum {
	GHOST_kDrawingContextTypeNone = 0 ,
	GHOST_kDrawingContextTypeOpenGL ,
	GHOST_kDrawingContextTypeD3D ,
} GHOST_TDrawingContextType;

typedef enum {
	GHOST_kEventUnknown = 0 ,

	GHOST_kEventCursorMove , /* Mouse move event. */
	GHOST_kEventButtonDown , /* Mouse button event. */
	GHOST_kEventButtonUp ,   /* Mouse button event. */
	GHOST_kEventWheel ,      /* Mouse wheel event. */

	GHOST_kEventKeyDown ,
	GHOST_kEventKeyUp ,

	GHOST_kEventQuitRequest ,

	GHOST_kEventWindowClose ,
	GHOST_kEventWindowUpdate ,
	GHOST_kEventWindowSize ,
	GHOST_kEventWindowMove ,

	GHOST_kEventTimer ,

	GHOST_kNumEventTypes
} GHOST_TEventType;

struct GHOST_EventMouseButton {
	GHOST_TButton button;
	int x , y , deltaWheel;
};

typedef enum { GHOST_kUseDefault = ( int ) 0x80000000 } GHOST_TCreateParams;

typedef struct { int32_t x , y; } GHOST_Point;
typedef struct { uint32_t cx , cy; } GHOST_Size;
typedef struct { int32_t left , top , right , bottom; } GHOST_Rect;

typedef enum { GHOST_kFailure = 0 , GHOST_kSuccess } GHOST_TStatus;
typedef enum { GHOST_kFireTimeNever = 0xffffffff } GHOST_TFireTimeConstant;

#ifdef __cplusplus
class GHOST_ITimerTask;
typedef void ( *GHOST_TimerProcPtr )( GHOST_ITimerTask *task , uint64_t time );
#else
struct GHOST_TimerTaskHandle__;
typedef void ( *GHOST_TimerProcPtr )( struct GHOST_TimerTaskHandle__ *task , uint64_t time );
#endif

typedef void *GHOST_TEventDataPtr;
typedef void *GHOST_UserDataPtr;

typedef long double GHOST_TimePoint;
