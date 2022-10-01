#pragma once

#include "lib_typed_buffer.h"
#include "lib_allocator.h"
#include "lib_utildefines.h"

#include <initializer_list>
#include <vector>
#include <array>
#include <cassert>

template<typename _Tp> class Span {
	const _Tp *mData = nullptr;
	size_t mSize = 0;
public:
	constexpr Span ( ) = default;

	constexpr Span ( const _Tp *start , size_t size ) : mData ( start ) , mSize ( size ) { }

	constexpr Span ( const std::initializer_list<_Tp> &list ) : Span ( list.begin ( ) , list.size ( ) ) { }

	constexpr Span ( const std::vector<_Tp> &vector ) : Span ( vector.data ( ) , vector.size ( ) ) { }

	template<size_t N> constexpr Span ( const std::array<_Tp , N> &array ) : Span ( array.data ( ) , N ) { }

	constexpr Span Slice ( ptrdiff_t start , ptrdiff_t size ) const {
		const ptrdiff_t new_size = ROSE_MAX ( 0 , ROSE_MIN ( size , this->mSize - start ) );
		return Span ( this->mData + start , new_size );
	}

	constexpr Span DropFront ( ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MAX ( 0 , this->mSize - n );
		return Span ( this->mData + n , new_size );
	}

	constexpr Span DropBack ( ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MAX ( 0 , this->mSize - n );
		return Span ( this->mData , new_size );
	}

	constexpr Span TakeFront ( ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MIN ( this->mSize , n );
		return Span ( this->mData , new_size );
	}

	constexpr Span TakeBack ( ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MIN ( this->mSize , n );
		return Span ( this->mData + this->mSize - new_size , new_size );
	}

	constexpr const _Tp *Data ( ) const {
		return this->mData;
	}

	constexpr const _Tp *Begin ( ) const {
		return this->mData;
	}

	constexpr const _Tp *End ( ) const {
		return this->mData + this->mSize;
	}

	constexpr const _Tp *begin ( ) const {
		return this->mData;
	}

	constexpr const _Tp *end ( ) const {
		return this->mData + this->mSize;
	}

	constexpr int64_t Size ( ) const {
		return this->mSize;
	}

	constexpr const _Tp &operator[]( size_t index ) const {
		return this->mData [ index ];
	}

	constexpr bool IsEmpty ( ) const {
		return this->mSize == 0;
	}

	constexpr bool Contains ( const _Tp &value ) const {
		for ( size_t index = 0; index < this->mSize; index++ ) {
			if ( this->mData [ index ] == value ) {
				return true;
			}
		}
		return false;
	}

	constexpr const _Tp &First ( ) const { return this->mData [ 0 ]; }
	constexpr const _Tp &Last ( ) const { return this->mData [ this->mSize - 1 ]; }

	constexpr _Tp Get ( size_t index , const _Tp &fallback ) const {
		if ( 0 <= index && index < this->mSize ) {
			return this->mData [ index ];
		}
		return fallback;
	}

	friend bool operator ==( const Span<_Tp> a , const Span<_Tp> b ) {
		if ( a.Size ( ) != b.Size ( ) ) {
			return false;
		}
		return std::equal ( a.Begin ( ) , a.End ( ) , b.Begin ( ) );
	}

	friend bool operator !=( const Span<_Tp> a , const Span<_Tp> b ) {
		return !( a == b );
	}
};

template<typename _Tp> class MutableSpan {
protected:
	_Tp *mData = nullptr;
	size_t mSize = 0;
public:
	constexpr MutableSpan ( ) = default;

	constexpr MutableSpan ( _Tp *start , const size_t size ) : mData ( start ) , mSize ( size ) { }

	template<size_t N> constexpr MutableSpan ( std::array<_Tp , N> &array ) : MutableSpan ( array.data ( ) , N ) { }

	template<typename U> constexpr MutableSpan ( MutableSpan<U> Span ) : mData ( static_cast< _Tp * >( Span.Data ( ) ) ) , mSize ( Span.Size ( ) ) { }

	template<typename U> constexpr operator Span<_Tp> ( ) const {
		return Span<_Tp> ( this->mData , this->mSize );
	}

	template<typename U> constexpr operator Span<U> ( ) const {
		return Span<U> ( static_cast< const U * >( this->mData ) , this->mSize );
	}

	constexpr size_t Size ( ) const {
		return this->mSize;
	}

	constexpr bool IsEmpty ( ) const {
		return this->mSize == 0;
	}

	constexpr void Fill ( const _Tp &value ) {
		initialized_fill_n ( this->mData , this->mSize , value );
	}

	constexpr void FillIndices ( Span<size_t> indices , const _Tp &value ) {
		for ( size_t i = 0; i < indices.Size ( ); i++ ) {
			this->mData [ indices [ i ] ] = value;
		}
	}

	constexpr _Tp *Data ( ) const {
		return this->mData;
	}

	constexpr _Tp *Begin ( ) const {
		return this->mData;
	}

	constexpr _Tp *End ( ) const {
		return this->mData + this->mSize;
	}

	constexpr _Tp &operator[]( const size_t index ) const {
		return this->mData [ index ];
	}

	constexpr MutableSpan Slice ( const size_t start , const size_t size ) const {
		const size_t new_size = ROSE_MAX ( 0 , ROSE_MIN ( size , this->mSize - start ) );
		return MutableSpan ( this->mData + start , new_size );
	}

	constexpr MutableSpan DropFont ( const ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MAX ( 0 , this->mSize - n );
		return MutableSpan ( this->mData + n , new_size );
	}

	constexpr MutableSpan DropBack ( const ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MAX ( 0 , this->mSize - n );
		return MutableSpan ( this->mData , new_size );
	}

	constexpr MutableSpan TakeFront ( const ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MIN ( this->mSize , n );
		return MutableSpan ( this->mData , new_size );
	}

	constexpr MutableSpan TakeBack ( const ptrdiff_t n ) const {
		const ptrdiff_t new_size = ROSE_MIN ( this->mSize , n );
		return MutableSpan ( this->mData + this->mSize - new_size , new_size );
	}

	constexpr Span<_Tp> AsSpan ( ) const {
		return Span<_Tp> ( this->mData , this->mSize );
	}

	constexpr const _Tp &First ( ) const { return this->mData [ 0 ]; }
	constexpr const _Tp &Last ( ) const { return this->mData [ this->mSize - 1 ]; }

	constexpr void CopyFrom ( Span<_Tp> values ) {
		assert ( this->mSize == values.Size ( ) );
		initialized_copy_n ( values.Data ( ) , this->mSize , this->mData );
	}

	constexpr size_t Count ( const _Tp &value ) const {
		size_t counter = 0;
		for ( size_t i = 0; i < this->mSize; i++ ) {
			if ( this->mData [ i ] == value ) {
				counter++;
			}
		}
		return counter;
	}

	friend bool operator ==( const MutableSpan<_Tp> a , const MutableSpan<_Tp> b ) {
		if ( a.Size ( ) != b.Size ( ) ) {
			return false;
		}
		return std::equal ( a.Begin ( ) , a.End ( ) , b.Begin ( ) );
	}

	friend bool operator !=( const MutableSpan<_Tp> a , const MutableSpan<_Tp> b ) {
		return !( a == b );
	}
};
