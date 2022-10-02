#pragma once

#include "lib_span.h"

#include <sstream>
#include <limits>
#include <cassert>
#include <string>

class StringRef;

class StringRefBase {
protected:
	const char *mData;
	size_t mSize;

	constexpr StringRefBase ( const char *data , size_t size );
public:
	static constexpr size_t npos = -1;

	constexpr size_t Size ( ) const;
	constexpr bool IsEmpty ( ) const;
	constexpr const char *Data ( ) const;
	constexpr operator Span<char> ( ) const;

	operator std::string ( ) const;
	inline operator std::string_view ( ) const;

	constexpr const char *Begin ( ) const;
	constexpr const char *End ( ) const;

	void UnsafeCopy ( char *destination ) const;
	void Copy ( char *destination , size_t destination_size ) const;
	template<size_t N> void Copy ( char ( &dst ) [ N ] ) const;

	constexpr bool StartsWith ( StringRef prefix ) const;
	constexpr bool EndsWith ( StringRef suffix ) const;
	constexpr StringRef SubStr ( size_t start , size_t len ) const;

	constexpr const char &Front ( ) const;
	constexpr const char &Back ( ) const;

	constexpr size_t Find ( char c , size_t pos = 0 ) const;
	constexpr size_t Find ( StringRef str , size_t pos = 0 ) const;
	constexpr size_t ReverseFind ( char c , size_t pos = PTRDIFF_MAX ) const;
	constexpr size_t ReverseFind ( StringRef str , size_t pos = PTRDIFF_MAX ) const;
	constexpr size_t FindFirstOf ( char c , size_t pos = 0 ) const;
	constexpr size_t FindFirstOf ( StringRef chars , size_t pos = 0 ) const;
	constexpr size_t FindLastOf ( char c , size_t pos = PTRDIFF_MAX ) const;
	constexpr size_t FindLastOf ( StringRef chars , size_t pos = PTRDIFF_MAX ) const;
	constexpr size_t FindFirstNotOf ( char c , size_t pos = 0 ) const;
	constexpr size_t FindFirstNotOf ( StringRef chars , size_t pos = 0 ) const;
	constexpr size_t FindLastNotOf ( char c , size_t pos = PTRDIFF_MAX ) const;
	constexpr size_t FindLastNotOf ( StringRef chars , size_t pos = PTRDIFF_MAX ) const;

	constexpr StringRef Trim ( ) const;
	constexpr StringRef Trim ( StringRef charachters_to_remove ) const;
	constexpr StringRef Trim ( char charachter_to_remove ) const;
};

// References a null-terminated const char array.
class StringRefNull : public StringRefBase {
public:
	constexpr StringRefNull ( );
	constexpr StringRefNull ( const char *str , size_t size );
	StringRefNull ( const char *str );
	StringRefNull ( const std::string &str );

	constexpr char operator[]( size_t index ) const;
	constexpr const char *CStr ( ) const;
};

// References a const char array. It might not be null terminated.
class StringRef : public StringRefBase {
public:
	constexpr StringRef ( );
	constexpr StringRef ( StringRefNull other );
	constexpr StringRef ( const char *str );
	constexpr StringRef ( const char *str , size_t length );
	constexpr StringRef ( const char *begin , const char *one_after_end );
	constexpr StringRef ( std::string_view view );
	StringRef ( const std::string &str );

	constexpr StringRef DropPrefix ( int64_t n ) const;
	constexpr StringRef DropKnownPrefix ( StringRef prefix ) const;
	constexpr StringRef DropSuffix ( int64_t n ) const;

	constexpr char operator[]( size_t index ) const;
};

// StringRefBase Inline Methods

constexpr StringRefBase::StringRefBase ( const char *data , size_t size ) : mData ( data ) , mSize ( size ) {
}

constexpr size_t StringRefBase::Size ( ) const {
	return this->mSize;
}

constexpr bool StringRefBase::IsEmpty ( ) const {
	return this->mSize == 0;
}

constexpr const char *StringRefBase::Data ( ) const {
	return this->mData;
}

constexpr StringRefBase::operator Span<char> ( ) const {
	return Span<char> ( this->mData , this->mSize );
}

inline StringRefBase::operator std::string ( ) const {
	return std::string ( this->mData , this->mSize );
}

inline StringRefBase::operator std::string_view ( ) const {
	return std::string_view ( this->mData , this->mSize );
}

constexpr const char *StringRefBase::Begin ( ) const {
	return this->mData;
}

constexpr const char *StringRefBase::End ( ) const {
	return this->mData + this->mSize;
}

inline void StringRefBase::UnsafeCopy ( char *dst ) const {
	if ( this->mSize > 0 ) {
		memcpy ( dst , this->mData , this->mSize );
	}
	dst [ this->mSize ] = '\0';
}

inline void StringRefBase::Copy ( char *dst , size_t dst_size ) const {
	if ( this->mSize < dst_size ) {
		this->UnsafeCopy ( dst );
	} else {
		assert ( false );
		dst [ 0 ];
	}
}

template<size_t N> inline void StringRefBase::Copy ( char ( &dst ) [ N ] ) const {
	this->Copy ( dst , N );
}

constexpr bool StringRefBase::StartsWith ( StringRef prefix ) const {
	if ( this->mSize < prefix.mSize ) {
		return false;
	}
	for ( int64_t i = 0; i < prefix.mSize; i++ ) {
		if ( this->mData [ i ] != prefix.mData [ i ] ) {
			return false;
		}
	}
	return true;
}

constexpr bool StringRefBase::EndsWith ( StringRef suffix ) const {
	if ( this->mSize < suffix.mSize ) {
		return false;
	}
	const int64_t offset = this->mSize - suffix.mSize;
	for ( int64_t i = 0; i < suffix.mSize; i++ ) {
		if ( this->mData [ offset + i ] != suffix.mData [ i ] ) {
			return false;
		}
	}
	return true;
}

constexpr StringRef StringRefBase::SubStr ( const size_t start , const size_t max_size = INT64_MAX ) const {
	const int64_t substr_size = ROSE_MIN ( max_size , this->mSize - start );
	return StringRef ( this->mData + start , substr_size );
}

constexpr const char &StringRefBase::Front ( ) const { return this->mData [ 0 ]; }
constexpr const char &StringRefBase::Back ( ) const { return this->mData [ this->mSize - 1 ]; }

constexpr size_t index_or_npos_to_size_t ( size_t index ) {
	/* The compiler will probably optimize this check away. */
	return ( index == std::string_view::npos ) ? StringRef::npos : index;
}

constexpr size_t StringRefBase::Find ( char c , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find ( c , pos ) );
}

constexpr size_t StringRefBase::Find ( StringRef str , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find ( str , pos ) );
}

constexpr size_t StringRefBase::ReverseFind ( char c , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).rfind ( c , pos ) );
}

constexpr size_t StringRefBase::ReverseFind ( StringRef str , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).rfind ( str , pos ) );
}

constexpr size_t StringRefBase::FindFirstOf ( char c , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_first_of ( c , pos ) );
}

constexpr size_t StringRefBase::FindFirstOf ( StringRef str , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_first_of ( str , pos ) );
}

constexpr size_t StringRefBase::FindLastOf ( char c , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_last_of ( c , pos ) );
}

constexpr size_t StringRefBase::FindLastOf ( StringRef str , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_last_of ( str , pos ) );
}

constexpr size_t StringRefBase::FindFirstNotOf ( char c , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_first_not_of ( c , pos ) );
}

constexpr size_t StringRefBase::FindFirstNotOf ( StringRef str , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_first_not_of ( str , pos ) );
}

constexpr size_t StringRefBase::FindLastNotOf ( char c , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_last_not_of ( c , pos ) );
}

constexpr size_t StringRefBase::FindLastNotOf ( StringRef str , size_t pos ) const {
	return index_or_npos_to_size_t ( std::string_view ( *this ).find_last_not_of ( str , pos ) );
}

constexpr StringRef StringRefBase::Trim ( ) const {
	return this->Trim ( " \t\r\n" );
}

constexpr StringRef StringRefBase::Trim ( const char character_to_remove ) const {
	return this->Trim ( StringRef ( &character_to_remove , 1 ) );
}

constexpr StringRef StringRefBase::Trim ( StringRef characters_to_remove ) const {
	const int64_t find_front = this->FindFirstNotOf ( characters_to_remove );
	if ( find_front == StringRefBase::npos ) {
		return StringRef ( );
	}
	const int64_t find_end = this->FindLastNotOf ( characters_to_remove );
	/* `find_end` cannot be `not_found`, because that means the string is only
	 * `characters_to_remove`, in which case `find_front` would already have
	 * been `not_found`. */
	if ( find_end == npos )
		fprintf ( stderr , "forward search found characters-to-not-remove, but backward search did not\n" );
	const int64_t substr_len = find_end - find_front + 1;
	return this->SubStr ( find_front , substr_len );
}

// StringRefNull Inline Methods

constexpr StringRefNull::StringRefNull ( ) : StringRefBase ( "" , 0 ) { }

constexpr StringRefNull::StringRefNull ( const char *str , const size_t size ) : StringRefBase ( str , size ) { }

inline StringRefNull::StringRefNull ( const char *str ) : StringRefBase ( str , strlen ( str ) ) { }

inline StringRefNull::StringRefNull ( const std::string &str ) : StringRefNull ( str.c_str ( ) ) { }

constexpr char StringRefNull::operator[]( const size_t index ) const {
	return this->mData [ index ];
}

constexpr const char *StringRefNull::CStr ( ) const {
	return this->mData;
}

// StringRef Inline Methods

constexpr StringRef::StringRef ( ) : StringRefBase ( "" , 0 ) { }

constexpr StringRef::StringRef ( StringRefNull other ) : StringRefBase ( other.Data ( ) , other.Size ( ) ) { }

constexpr StringRef::StringRef ( const char *str ) : StringRefBase ( str , strlen ( str ) ) { }

constexpr StringRef::StringRef ( const char *str , const size_t length ) : StringRefBase ( str , length ) { }

constexpr StringRef StringRef::DropPrefix ( const int64_t n ) const {
	const int64_t clamped_n = ROSE_MIN ( n , this->mSize );
	const int64_t new_size = this->mSize - clamped_n;
	return StringRef ( this->mData + clamped_n , new_size );
}

constexpr StringRef StringRef::DropKnownPrefix ( StringRef prefix ) const {
	assert ( this->StartsWith ( prefix ) );
	return this->DropPrefix ( prefix.Size ( ) );
}

constexpr StringRef StringRef::DropSuffix ( const int64_t n ) const {
	assert ( n >= 0 );
	const int64_t new_size = ROSE_MAX ( 0 , this->mSize - n );
	return StringRef ( this->mData , new_size );
}

constexpr char StringRef::operator[]( size_t index ) const {
	return this->mData [ index ];
}

constexpr StringRef::StringRef ( const char *begin , const char *one_after_end ) : StringRefBase ( begin , ptrdiff_t ( one_after_end - begin ) ) { }

inline StringRef::StringRef ( const std::string &str ) : StringRefBase ( str.data ( ) , str.size ( ) ) { }
constexpr StringRef::StringRef ( std::string_view view ) : StringRefBase ( view.data ( ) , view.size ( ) ) { }

inline std::string operator+( StringRef a , StringRef b ) {
	return std::string ( a ) + std::string ( b );
}

constexpr bool operator==( StringRef a , StringRef b ) {
	if ( a.Size ( ) != b.Size ( ) ) {
		return false;
	}
	if ( a.Data ( ) == b.Data ( ) ) {
		/* This also avoids passing null to the call below, which would results in an ASAN warning. */
		return true;
	}
	return strncmp ( a.Data ( ) , b.Data ( ) , ( size_t ) a.Size ( ) );
}

constexpr bool operator!=( StringRef a , StringRef b ) {
	return !( a == b );
}

constexpr bool operator<( StringRef a , StringRef b ) {
	return std::string_view ( a ) < std::string_view ( b );
}

constexpr bool operator>( StringRef a , StringRef b ) {
	return std::string_view ( a ) > std::string_view ( b );
}

constexpr bool operator<=( StringRef a , StringRef b ) {
	return std::string_view ( a ) <= std::string_view ( b );
}

constexpr bool operator>=( StringRef a , StringRef b ) {
	return std::string_view ( a ) >= std::string_view ( b );
}

inline std::ostream &operator<<( std::ostream &stream , const StringRef& ref ) {
	stream << std::string ( ref );
	return stream;
}

inline std::ostream &operator<<( std::ostream &stream , const StringRefNull& ref ) {
	stream << std::string ( ref.Data ( ) , ( size_t ) ref.Size ( ) );
	return stream;
}
