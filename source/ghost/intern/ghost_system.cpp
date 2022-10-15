#include "ghost_system.h"

#include <chrono>

static uint64_t TimeSinceEpoch ( ) {
	return std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::system_clock::now ( ).time_since_epoch ( ) ).count ( );
}

GHOST_TimePoint GHOST_System::GetTime ( ) const {
	return ( GHOST_TimePoint ) std::chrono::duration<double> ( std::chrono::system_clock::now ( ) - std::chrono::system_clock::from_time_t ( this->mStartTime ) ).count ( );
}

uint64_t GHOST_System::GetMilliseconds ( ) const {
	return std::chrono::duration_cast<std::chrono::milliseconds> ( std::chrono::system_clock::now ( ) - std::chrono::system_clock::from_time_t ( this->mStartTime ) ).count ( );
}

GHOST_IntTimerTask *GHOST_System::InstallTimer ( uint64_t delay , uint64_t interval , GHOST_TimerProcPtr proc , GHOST_UserDataPtr user ) {
	uint64_t ms = GetMilliseconds ( );
	GHOST_TimerTask *task = new GHOST_TimerTask ( ms + delay , interval , proc , user );
	if ( task ) {
		if ( this->mTimerManager->AddTimer ( task ) == GHOST_kSuccess ) {
			this->mTimerManager->FireTimers ( ms );
		} else {
			delete task; task = nullptr;
		}
	}
	return task;
}

GHOST_TStatus GHOST_System::RemoveTimer ( GHOST_IntTimerTask *timer ) {
	return this->mTimerManager->RemoveTimer ( ( GHOST_TimerTask * ) timer );
}

GHOST_TStatus GHOST_System::DisposeWindow ( GHOST_IntWindow *window ) {
	if ( window && this->mWindowManager->GetWindowFound ( ( GHOST_Window * ) window ) ) {
		this->mWindowManager->RemoveWindow ( ( GHOST_Window * ) window );
		delete window;
		return GHOST_kSuccess;
	}
	return GHOST_kFailure;
}

GHOST_TStatus GHOST_System::Init ( ) {
	this->mStartTime = std::chrono::system_clock::to_time_t ( std::chrono::system_clock::now ( ) );
	this->mShouldExit = false;
	return GHOST_IntSystem::Init ( );
}

GHOST_TStatus GHOST_System::Exit ( ) {
	if ( this->mWindowManager ) {
		while ( this->mWindowManager->GetWindows ( ).size ( ) ) {
			this->DisposeWindow ( this->mWindowManager->GetWindows ( ) [ 0 ] );
		}
	}
	return GHOST_IntSystem::Exit ( );
}

void GHOST_System::DispatchEvents ( ) {
	this->mEventManager->DispatchEvents ( );
}

GHOST_TStatus GHOST_System::AddConsumer ( GHOST_IntEventConsumer *consumer ) {
	return this->mEventManager->AddConsumer ( consumer );
}

GHOST_TStatus GHOST_System::RemoveConsumer ( GHOST_IntEventConsumer *consumer ) {
	return this->mEventManager->RemoveConsumer ( consumer );
}
