#include "ghost_timermanager.h"

#include <algorithm>

GHOST_TimerManager::GHOST_TimerManager ( ) {

}

GHOST_TimerManager::~GHOST_TimerManager ( ) {
	DisposeTimers ( );
}

bool GHOST_TimerManager::HasTimer ( GHOST_TimerTask *timer ) const {
	return std::find ( this->mTasks.begin ( ) , this->mTasks.end ( ) , timer ) != this->mTasks.end ( );
}

GHOST_TStatus GHOST_TimerManager::AddTimer ( GHOST_TimerTask *timer ) {
	if ( std::find ( this->mTasks.begin ( ) , this->mTasks.end ( ) , timer ) == this->mTasks.end ( ) ) {
		this->mTasks.push_back ( timer );
		return GHOST_kSuccess;
	}
	return GHOST_kFailure;
}

GHOST_TStatus GHOST_TimerManager::RemoveTimer ( GHOST_TimerTask *timer ) {
	std::vector<GHOST_TimerTask *>::iterator itr = std::find ( this->mTasks.begin ( ) , this->mTasks.end ( ) , timer );
	if ( itr != this->mTasks.end ( ) ) {
		this->mTasks.erase ( itr ); delete timer;
		return GHOST_kSuccess;
	}
	return GHOST_kFailure;
}

uint64_t GHOST_TimerManager::GetNextFireTime ( ) const {
	uint64_t smallest = GHOST_kFireTimeNever;
	for ( std::vector<GHOST_TimerTask *>::const_iterator iter = this->mTasks.begin ( ); iter != this->mTasks.end ( ); ++iter ) {
		smallest = std::min ( ( *iter )->GetNext ( ) , smallest );
	}
	return smallest;
}

bool GHOST_TimerManager::FireTimers ( uint64_t time ) {
	bool anyProcessed = false;

	for ( std::vector<GHOST_TimerTask *>::iterator iter = this->mTasks.begin ( ); iter != this->mTasks.end ( ); ++iter ) {
		if ( FireTimer ( time , *iter ) ) {
			anyProcessed = true;
		}
	}

	return anyProcessed;
}

bool GHOST_TimerManager::FireTimer ( uint64_t time , GHOST_TimerTask *task ) {
	uint64_t next = task->GetNext ( );

	if ( time > next ) { // Check if the timer should be fired.
		GHOST_TimerProcPtr TimerProc = task->GetTimerProc ( );
		uint64_t start = task->GetStart ( );
		TimerProc ( ( GHOST_ITimerTask * ) task , time - start );

		uint64_t interval = task->GetInterval ( ); // Update the time at which we will fire it again.

		if ( interval != GHOST_kFireTimeNever ) {
			uint64_t nCalls = ( interval ) ? ( ( next - start ) / interval ) : 0 + 1ull;
			next = start + nCalls * interval;
			task->SetNext ( next );
		} else {
			this->RemoveTimer ( task );
		}
		return true;
	}
	return false;
}

void GHOST_TimerManager::DisposeTimers ( ) {
	for ( std::vector<GHOST_TimerTask *>::iterator iter = this->mTasks.begin ( ); iter != this->mTasks.end ( ); ++iter ) {
		delete ( *iter );
	}
	this->mTasks.clear ( );
}
