#pragma once

#include "ghost/ghost_itimertask.h"

class GHOST_TimerTask : public GHOST_IntTimerTask {
public:
	/** Create a new timer task descriptor.
	* \param start The timer task time.
	* \param interval The timer intval between calls to the #callback procedure function.
	* \param callback The timer user callback function.
	* \param data The custom user data for the timer task returned to the user #callback. */
	GHOST_TimerTask ( uint64_t start , uint64_t interval , GHOST_TimerProcPtr callback , GHOST_UserDataPtr data ) {
		this->mStart = start;
		this->mNext = start;
		this->mInterval = interval;
		this->mCallback = callback;
		this->mUserData = data;
	}

	//! Retrieves the timer start time.
	inline uint64_t GetStart ( ) const { return this->mStart; }

	//! Retrieves the time the timer callback proc will be called.
	inline uint64_t GetNext ( ) const { return this->mNext; }

	//! Retrieves the timer inverval.
	inline uint64_t GetInterval ( ) const { return this->mInterval; }

	//! Retrieves the timer callback.
	inline GHOST_TimerProcPtr GetTimerProc ( ) const { return this->mCallback; }

	//! Returns the timer user data.
	inline GHOST_UserDataPtr GetUserData ( ) const { return this->mUserData; }

	//! Changes the auxilary storage room value.
	inline uint32_t GetAuxData ( ) const { return this->mAuxData; }

	//! Changes the timer start time, to the specified value.
	inline void SetStart ( uint64_t start ) { this->mStart = start; }

	//! Changes the time the timer callback proc will be called.
	inline void SetNext ( uint64_t next ) { this->mNext = next; }

	//! Changes the timer interval time, to the specified value.
	inline void SetInterval ( uint64_t interval ) { this->mInterval = interval; }

	//! Changes the timer callback to the specified function.
	inline void SetTimerProc ( GHOST_TimerProcPtr callback ) { this->mCallback = callback; }

	//! Changes the timer user data.
	inline void SetUserData ( GHOST_UserDataPtr data ) { this->mUserData = data; }

	//! Changes the auxilary storage room value.
	inline void SetAuxData ( uint32_t data ) { this->mAuxData = data; }
protected:
	uint64_t mStart;
	uint64_t mNext;
	uint64_t mInterval;
	GHOST_TimerProcPtr mCallback;
	GHOST_UserDataPtr mUserData;

	uint32_t mAuxData;
};
