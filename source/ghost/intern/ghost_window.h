#pragma once

#include "ghost/ghost_iwindow.h"

#include "ghost_context.h"
#include "ghost_contextnone.h"

class GHOST_Window : public GHOST_IntWindow {
public:
	// Constructor.
	GHOST_Window ( );

	// Destructor.
	virtual ~GHOST_Window ( );

	/** Retrieves the position of the window.
	* \return Returns a GHOST_Point structure describint the position of the top left window's point. */
	virtual GHOST_Point GetWindowPoint ( ) const {
		GHOST_Rect rect = GetWindowBounds ( );
		return { int32_t ( rect.left ) , int32_t ( rect.top ) };
	}

	/** Retrieves the size of the window's area.
	* \return Returns a GHOST_Size structure describint the size of the area of the window. */
	virtual GHOST_Size GetWindowSize ( ) const {
		GHOST_Rect rect = GetWindowBounds ( );
		return { uint32_t ( rect.right - rect.left ) , uint32_t ( rect.bottom - rect.top ) };
	}

	/** Retrieves the size of the window's client area.
	* \return Returns a GHOST_Size structure describint the size of the client area of the window. */
	inline virtual GHOST_Size GetClientSize ( ) const {
		GHOST_Rect rect = GetClientBounds ( );
		return { uint32_t ( rect.right - rect.left ) , uint32_t ( rect.bottom - rect.top ) };
	}

	/** Returns the user data assigned to the window.
	* \return The user data. */
	GHOST_UserDataPtr GetUserData ( ) const;

	/** Updates the window's user data.
	* \param data The new user data for the window. */
	void SetUserData ( GHOST_UserDataPtr data );
	
	/** Returns the type of drawing context used in this window.
	* \return The current type of drawing context. */
	GHOST_TDrawingContextType GetDrawingContextType ( ) const { return this->mDrawingContextType; }

	/** Tries to install a rendering context in this window.
	* \param type The type of rendering context to be isntalled.
	* \return Indication as to whether installation has succeeded. */
	GHOST_TStatus SetDrawingContextType ( GHOST_TDrawingContextType type );

	/** Swaps front and back buffers of a window.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus SwapBuffers ( ) { return this->mContext->SwapBuffers ( ); }

	/** Sets the swap interval for #SwapBuffers.
	* \param interval The swap interval to use.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus SetSwapInterval ( int interval ) const { return this->mContext->SetSwapInterval ( interval ); }

	/** Gets the current swap interval for #SwapBuffers.
	* \param interval pointer to location to return swap interval.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus GetSwapInterval ( int *interval ) const { return this->mContext->GetSwapInterval ( interval ); }

	/** Activates the drawing context of this window.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus ActivateDrawingContext ( ) { return this->mContext->ActivateDrawingContext ( ); }

	/** Makes the specified rendering context so that it's no longer current.
	* All subsequent context calls made by the thread are drawn on the device identified.
	* \return If the function succeds the return value is GHOST_tStatus::GHOST_kSuccess
	* a nonzero value, otherwise the return value is GHOST_tStatus::GHOST_kFailure
	* indicating failure. */
	GHOST_TStatus ReleaseDrawingContext ( ) { return this->mContext->ReleaseDrawingContext ( ); }
protected:
	/** Tries to install a rendering context in this window.
	* \param type: The type of rendering context installed.
	* \return Indication as to whether installation has succeeded. */
	virtual GHOST_Context *NewDrawingContext ( GHOST_TDrawingContextType type ) = 0;

	// Release native window handles.
	GHOST_TStatus ReleaseNativeHandles ( );

	GHOST_TDrawingContextType mDrawingContextType;
	GHOST_UserDataPtr mUserData;
private:
	GHOST_Context *mContext;
};
