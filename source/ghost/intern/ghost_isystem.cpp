#include "ghost/ghost_isystem.h"

#include "ghost_system.h"

#ifdef _WIN32
#  include "ghost_systemwin32.h"
#else
#  include "ghost_system.h"
#endif

GHOST_IntSystem *GHOST_IntSystem::sGhostSystem = nullptr;

GHOST_TStatus GHOST_IntSystem::CreateSystem ( ) {
	if ( !sGhostSystem ) {
#ifdef _WIN32
		sGhostSystem = ( GHOST_IntSystem * ) new GHOST_SystemWin32 ( );
#else
		sGhostSystem = ( GHOST_IntSystem * ) new GHOST_System ( );
#endif

		// Init the common managers here.
		sGhostSystem->mTimerManager = new GHOST_TimerManager ( );
		sGhostSystem->mEventManager = new GHOST_EventManager ( );
		sGhostSystem->mWindowManager = nullptr;

		if ( sGhostSystem ) {
			if ( sGhostSystem->Init ( ) != GHOST_kSuccess ) {
				delete sGhostSystem; sGhostSystem = nullptr;
			}
		}
		return ( sGhostSystem ) ? GHOST_kSuccess : GHOST_kFailure;
	}
	return GHOST_kFailure;
}

GHOST_IntSystem *GHOST_IntSystem::GetSystem ( ) {
	return GHOST_IntSystem::sGhostSystem;
}

GHOST_TStatus GHOST_IntSystem::DisposeSystem ( ) {
	if ( sGhostSystem ) {
		if ( sGhostSystem->Exit ( ) == GHOST_kSuccess ) {
			// Delete the managers here.
			delete sGhostSystem->mTimerManager; sGhostSystem->mTimerManager = nullptr;
			delete sGhostSystem->mWindowManager; sGhostSystem->mWindowManager = nullptr;
			delete sGhostSystem->mEventManager; sGhostSystem->mEventManager = nullptr;

			delete sGhostSystem; sGhostSystem = nullptr;
			return GHOST_kSuccess;
		}
	}
	return GHOST_kFailure;
}
