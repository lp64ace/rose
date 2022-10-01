#pragma once

#include "lib_typed_buffer.h"
#include "lib_allocator.h"
#include "lib_utildefines.h"

#include "lib_span.h"

template<typename _Tp , size_t _InlineBufferCapacity = 16 , typename _Allocator = Allocator> class Vector {
public:
	using value_type = _Tp;
	using pointer = _Tp *;
	using const_pointer = const _Tp *;
	using reference = _Tp &;
	using const_reference = const _Tp &;
	using iterator = _Tp *;
	using const_iterator = const _Tp *;
private:
	_Tp *mBegin;
	_Tp *mEnd;
	_Tp *mEndCapacity;

	Allocator mAllocator;
	TypedBuffer<_Tp , _InlineBufferCapacity> mInlineBuffer;
public:
	inline Vector ( Allocator allocator = {} ) : mAllocator ( allocator ) {
		this->mBegin = mInlineBuffer;
		this->mEnd = mInlineBuffer;
		this->mEndCapacity = this->mBegin + _InlineBufferCapacity;
	}

	explicit Vector ( int64_t size , Allocator allocator = {} ) : Vector ( allocator ) {
		this->Resize ( size );
	}

	Vector ( int64_t size , const _Tp &value , Allocator allocator = {} ) : Vector ( allocator ) {
		this->Resize ( size , value );
	}

	Vector ( const std::initializer_list<_Tp> &values ) : Vector ( Span<_Tp> ( values ) ) { }

	template<typename U> Vector ( Span<U> values , Allocator allocator = {} ) : Vector ( allocator ) {
		const int64_t size = values.Size ( );
		this->Reserve ( size );
		uninitialized_convert_n<U , _Tp> ( values.Data ( ) , size , this->mBegin );
		this->IncreaseSizeByUnchecked ( size );
	}

	Vector ( const Vector &other ) : Vector ( other.AsSpan ( ) , other.mAllocator ) { }

	void Insert ( const size_t insert_index , const _Tp &value ) {
		this->Insert ( insert_index , Span<_Tp> ( &value , 1 ) );
	}

	void Insert ( const size_t insert_index , _Tp &&value ) {
		this->Insert ( insert_index , std::make_move_iterator ( &value ) , std::make_move_iterator ( &value + 1 ) );
	}

	void Insert ( const size_t insert_index , Span<_Tp> array ) {
		this->Insert ( this->mBegin + insert_index , array.Begin ( ) , array.End ( ) );
	}

	template<typename InputIt> void Insert ( const _Tp *insert_position , InputIt first , InputIt last ) {
		const size_t insert_index = insert_position - this->mBegin;
		this->Insert ( insert_index , first , last );
	}

	template<typename InputIt> void Insert ( const size_t insert_index , InputIt first , InputIt last ) {
		const int64_t insert_amount = std::distance ( first , last );
		const int64_t old_size = this->Size ( );
		const int64_t new_size = old_size + insert_amount;
		const int64_t move_amount = old_size - insert_index;

		this->Reserve ( new_size );
		for ( int64_t i = 0; i < move_amount; i++ ) {
			const int64_t src_index = insert_index + move_amount - i - 1;
			const int64_t dst_index = new_size - i - 1;
			try {
				new ( static_cast< void * >( this->mBegin + dst_index ) ) _Tp ( std::move ( this->mBegin [ src_index ] ) );
			} catch ( ... ) {
				/* Destruct all values that have been moved already. */
				destruct_n ( this->mBegin + dst_index + 1 , i );
				this->mEnd = this->mBegin + src_index + 1;
				throw;
			}
			this->mBegin [ src_index ].~_Tp ( );
		}

		try {
			std::uninitialized_copy_n ( first , insert_amount , this->mBegin + insert_index );
		} catch ( ... ) {
			/* Destruct all values that have been moved. */
			destruct_n ( this->mBegin + new_size - move_amount , move_amount );
			this->mEnd = this->mBegin + insert_index;
			throw;
		}
		this->mEnd = this->mBegin + new_size;
	}

	inline void RemoveLast ( ) {
		this->mEnd--;
		this->mEnd->~T ( );
	}

	inline _Tp PopLast ( ) {
		_Tp value = std::move ( *( this->mEnd - 1 ) );
		this->mEnd--;
		this->mEnd->~T ( );
		return value;
	}

	inline void Remove ( const int64_t index ) {
		const int64_t last_index = this->Size ( ) - 1;
		for ( int64_t i = index; i < last_index; i++ ) {
			this->mBegin [ i ] = std::move ( this->mBegin [ i + 1 ] );
		}
		this->mBegin [ last_index ].~T ( );
		this->mEnd--;
	}

	inline void Remove ( const size_t start_index , const size_t amount ) {
		if ( start_index + amount > this->Size ( ) ) {
			amount = this->Size ( ) - start_index;
		}
		const size_t old_size = this->Size ( );
		const ptrdiff_t  move_amount = old_size - start_index - amount;
		for ( ptrdiff_t i = 0; i < move_amount; i++ ) {
			this->mBegin [ start_index + i ] = std::move ( this->mBegin [ start_index + amount + i ] );
		}
		destruct_n ( this->mEnd - amount , amount );
		this->mEnd -= amount;
	}

	inline void Append ( const _Tp &value ) {
		this->AppendAs ( value );
	}

	inline void Append ( _Tp &&value ) {
		this->AppendAs ( std::move ( value ) );
	}

	template<typename... ForwardValue> void AppendAs ( ForwardValue &&...value ) {
		this->EnsureSpaceForOne ( );
		this->AppendUncheckedAs ( std::forward<ForwardValue> ( value )... );
	}

	void AppendUnchecked ( const _Tp &value ) {
		this->AppendUncheckedAs ( value );
	}

	void AppendUnchecked ( _Tp &&value ) {
		this->AppendUncheckedAs ( std::move ( value ) );
	}

	template<typename... ForwardT> void AppendUncheckedAs ( ForwardT &&...value ) {
		new ( this->mEnd ) _Tp ( std::forward<ForwardT> ( value )... );
		this->mEnd++;
	}

	inline const _Tp &operator[]( int64_t index ) const { return this->mBegin [ index ]; }
	inline _Tp &operator[]( int64_t index ) { return this->mBegin [ index ]; }

	inline void Fill ( const _Tp &value ) const {
		initialized_fill_n ( this->mBegin , this->Size ( ) , value );
	}

	inline const _Tp *Data ( ) const { return this->mBegin; }
	inline _Tp *Data ( ) { return this->mBegin; }

	inline const _Tp *Begin ( ) const { return this->mBegin; }
	inline const _Tp *End ( ) const { return this->mEnd; }

	inline _Tp *Begin ( ) { return this->mBegin; }
	inline _Tp *End ( ) { return this->mEnd; }

	inline const_iterator begin ( ) const { return this->mBegin; }
	inline const_iterator end ( ) const { return this->mEnd; }

	inline iterator begin ( ) { return this->mBegin; }
	inline iterator end ( ) { return this->mEnd; }

	inline void Reserve ( const size_t capacity ) {
		if ( capacity > this->Capacity ( ) ) {
			this->ReallocToAtLeast ( capacity );
		}
	}

	inline void Resize ( const size_t new_size ) {
		const int64_t old_size = this->Size ( );
		if ( new_size > old_size ) {
			this->Reserve ( new_size );
			default_construct_n ( this->mBegin + old_size , new_size - old_size );
		} else {
			destruct_n ( this->mBegin + new_size , old_size - new_size );
		}
		this->mEnd = this->mBegin + new_size;
	}

	inline void Resize ( const size_t new_size , const _Tp &value ) {
		const size_t old_size = this->Size ( );
		if ( new_size > old_size ) {
			this->Reserve ( new_size );
			uninitialized_fill_n ( this->mBegin + old_size , new_size - old_size , value );
		} else {
			destruct_n ( this->mBegin + new_size , old_size - new_size );
		}
		this->mEnd = this->mBegin + new_size;
	}

	inline size_t Size ( ) const {
		return size_t ( this->mEnd - this->mBegin );
	}

	inline size_t Capacity ( ) const {
		return size_t ( this->mEndCapacity - this->mBegin );
	}

	inline void Clear ( ) {
		destruct_n ( this->mBegin , this->Size ( ) );
		this->mEnd = this->mBegin;
	}

	inline bool IsEmpty ( ) const {
		return this->mEnd == this->mBegin;
	}

	operator Span<_Tp> ( ) const {
		return Span<_Tp> ( this->mBegin , this->Size ( ) );
	}

	operator MutableSpan<_Tp> ( ) const {
		return MutableSpan<_Tp> ( this->mBegin , this->Size ( ) );
	}

	Span<_Tp> AsSpan ( ) const {
		return *this;
	}

	MutableSpan<_Tp> AsMutableSpan ( ) {
		return *this;
	}

	friend bool operator==( const Vector &a , const Vector &b ) {
		return a.AsSpan ( ) == b.AsSpan ( );
	}

	friend bool operator!=( const Vector &a , const Vector &b ) {
		return !( a == b );
	}

	void Extend ( Span<_Tp> array ) {
		this->Extend ( array.Data ( ) , array.Size ( ) );
	}

	void Extend ( const _Tp *start , size_t amount ) {
		this->Reserve ( this->Size ( ) + amount );
		this->ExtendUnchecked ( start , amount );
	}

	void ExtendNoNDuplicates ( Span<_Tp> array ) {
		for ( size_t i = 0; i < array.Size ( ); i++ ) {
			this->AppendNoNDuplicates ( array [ i ] );
		}
	}

	void ExtendUnchecked ( Span<_Tp> array ) {
		this->ExtendUnchecked ( array.Data ( ) , array.Size ( ) );
	}

	void ExtendUnchecked ( const _Tp *start , size_t amount ) {
		uninitialized_copy_n ( start , amount , this->mEnd );
		this->mEnd += amount;
	}

	void AppendNoNDuplicates ( const _Tp &value ) {
		if ( !this->Contains ( value ) ) {
			this->Append ( value );
		}
	}

	size_t FirstIndexOfTry ( const _Tp &value ) const {
		for ( const _Tp *current = this->mBegin; current != this->mEnd; current++ ) {
			if ( *current == value ) {
				return static_cast< size_t >( current - this->mBegin );
			}
		}
		return -1;
	}

	bool Contains ( const _Tp &value ) const {
		return this->FirstIndexOfTry ( value ) != -1;
	}
private:
	void IncreaseSizeByUnchecked ( const int64_t n ) noexcept {
		this->mEnd += n;
	}

	bool IsInline ( void ) const {
		return this->mBegin == this->mInlineBuffer;
	}

	void EnsureSpaceForOne ( ) {
		if ( this->mEnd >= this->mEndCapacity ) {
			this->ReallocToAtLeast ( this->Size ( ) + 1 );
		}
	}

	void ReallocToAtLeast ( const size_t capacity ) {
		if ( this->Capacity ( ) >= capacity ) {
			return;
		}

		const size_t min_new_capacity = this->Capacity ( ) * 2;

		const size_t new_capacity = ROSE_MAX ( capacity , min_new_capacity );
		const size_t size = this->Size ( );

		_Tp *new_array = ( _Tp * ) ( this->mAllocator.Allocate ( ( size_t ) ( new_capacity ) * sizeof ( _Tp ) ) );
		try {
			uninitialized_relocate_n ( this->mBegin , size , new_array );
		} catch ( ... ) {
			this->mAllocator.Deallocate ( new_array ); new_array = nullptr;
			throw;
		}

		if ( !this->IsInline ( ) ) {
			this->mAllocator.Deallocate ( this->mBegin ); this->mBegin = nullptr;
		}

		this->mBegin = new_array;
		this->mEnd = this->mBegin + size;
		this->mEndCapacity = this->mBegin + new_capacity;
	}
};
