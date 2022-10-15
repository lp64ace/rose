#pragma once

#include "lib_typedef.h"
#include "lib_mem_utils.h"

template<size_t Size , size_t Alignment> class AlignedBuffer {
	struct Empty { };
	struct alignas( Alignment ) Sized { byte buffer_ [ Size > 0 ? Size : 1 ]; };

	using BufferType = std::conditional_t<Size == 0 , Empty , Sized>;
	BufferType buffer_;
public:
	operator void *( ) { return this; }

	operator const void *( ) const { return this; }

	void *ptr ( ) { return this; }

	const void *ptr ( ) const { return this; }
};

template<typename _Tp , size_t Size = 1> class TypedBuffer {
private:
	AlignedBuffer<sizeof ( _Tp ) * Size , alignof( _Tp )> buffer_;
public:
	operator _Tp *( ) { return static_cast< _Tp * >( buffer_.ptr ( ) ); }

	operator const _Tp *( ) const { return static_cast< const _Tp * >( buffer_.ptr ( ) ); }

	_Tp &operator*( ) { return *static_cast< _Tp * >( buffer_.ptr ( ) ); }

	const _Tp &operator*( ) const { return *static_cast< const _Tp * >( buffer_.ptr ( ) ); }

	_Tp *ptr ( ) { return static_cast< _Tp * >( buffer_.ptr ( ) ); }

	const _Tp *ptr ( ) const { return static_cast< const _Tp * >( buffer_.ptr ( ) ); }

	_Tp &ref ( ) { return *static_cast< _Tp * >( buffer_.ptr ( ) ); }

	const _Tp &ref ( ) const { return *static_cast< const _Tp * >( buffer_.ptr ( ) ); }
};
