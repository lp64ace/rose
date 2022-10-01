#include "ghost_window.h"

GHOST_Window::GHOST_Window ( ) {
	this->mUserData = NULL;

	this->mContext = new GHOST_ContextNone ( );
	this->mDrawingContextType = GHOST_kDrawingContextTypeNone;
}

GHOST_Window::~GHOST_Window ( ) {
	delete this->mContext; this->mContext = nullptr;
}

GHOST_UserDataPtr GHOST_Window::GetUserData ( ) const {
	return this->mUserData;
}

void GHOST_Window::SetUserData ( GHOST_UserDataPtr data ) {
	this->mUserData = data;
}

GHOST_TStatus GHOST_Window::SetDrawingContextType ( GHOST_TDrawingContextType type ) {
	if ( type != this->mDrawingContextType ) {
		delete this->mContext; this->mContext = nullptr;
		if ( type != GHOST_kDrawingContextTypeNone ) {
			this->mContext = NewDrawingContext ( type );
		}
		if ( this->mContext != nullptr ) {
			this->mDrawingContextType = type;
		} else {
			this->mContext = new GHOST_ContextNone ( );
			this->mDrawingContextType = GHOST_kDrawingContextTypeNone;
		}

		return ( type == this->mDrawingContextType ) ? GHOST_kSuccess : GHOST_kFailure;
	}
	return GHOST_kSuccess;
}

GHOST_TStatus GHOST_Window::ReleaseNativeHandles ( ) {
	return this->mContext->ReleaseNativeHandles ( );
}
