#include "ghost_windowmanager.h"

GHOST_WindowManager::GHOST_WindowManager ( ) {
}

GHOST_WindowManager::~GHOST_WindowManager ( ) {
}

GHOST_TStatus GHOST_WindowManager::AddWindow ( GHOST_Window *window ) {
	if ( window ) {
		if ( !GetWindowFound ( window ) ) {
			this->mWindows.push_back ( window );
			return GHOST_kSuccess;
		}
	}
	return GHOST_kFailure;
}

GHOST_TStatus GHOST_WindowManager::RemoveWindow ( const GHOST_Window *window ) {
	if ( window ) {
		if ( GetWindowFound ( window ) ) {
			std::vector<GHOST_Window *>::iterator itr = std::find ( this->mWindows.begin ( ) , this->mWindows.end ( ) , window );
			if ( itr != this->mWindows.end ( ) ) {
				this->mWindows.erase ( itr );
				return GHOST_kSuccess;
			}
		}
	}
	return GHOST_kFailure;
}

bool GHOST_WindowManager::GetWindowFound ( const GHOST_Window *window ) const {
	std::vector<GHOST_Window *>::const_iterator itr = std::find ( this->mWindows.begin ( ) , this->mWindows.end ( ) , window );
	if ( itr != this->mWindows.end ( ) ) {
		return true;
	}
	return false;
}

const std::vector<GHOST_Window *> &GHOST_WindowManager::GetWindows ( ) const {
	return this->mWindows;
}
