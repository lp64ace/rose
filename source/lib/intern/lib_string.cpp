#define _CRT_SECURE_NO_WARNINGS
#include "lib/lib_string.h"
#include "lib/lib_alloc.h"

#include "utf/utfconv.h"

#include <stdarg.h>
#include <stdio.h>

String::String ( ) {
	this->mSize = 0;
	this->mAllocated = 0;
	this->mBuffer = nullptr;
	this->Resize ( 0 );
}

String::String ( char c ) : String ( ) {
	this->Append ( c );
}

String::String ( wchar_t c ) : String ( ) {
	this->Append ( c );
}

String::String ( const String &string ) : String ( ) {
	this->Append ( string.WCStr ( ) );
}

String::String ( const char *string ) : String ( ) {
	this->Append ( string );
}

String::String ( const char *string , size_t len ) : String ( ) {
	this->Append ( string , len );
}

String::String ( const wchar_t *string ) : String ( ) {
	this->Append ( string );
}

String::String ( const wchar_t *string , size_t len ) : String ( ) {
	this->Append ( string , len );
}

String::String ( std::initializer_list<String> list ) : String ( ) {
	for ( size_t i = 0; i < list.size ( ); i++ ) {
		this->Append ( list.begin ( ) [ i ] );
	}
}

String::~String ( ) {
	MEM_SAFE_FREE ( this->mBuffer );
}

const wchar_t &String::Back ( ) const { return this->mBuffer [ this->mSize - 1 ]; }
wchar_t &String::Back ( ) { return this->mBuffer [ this->mSize - 1 ]; }

bool String::StartsWith ( const char prefix ) const {
	return this->mBuffer [ 0 ] == ( wchar_t ) prefix;
}

bool String::StartsWith ( const wchar_t prefix ) const {
	return this->mBuffer [ 0 ] == prefix;
}

bool String::StartsWith ( const char *prefix ) const {
	size_t prefix_size = strlen ( prefix );
	if ( this->mSize < prefix_size ) {
		return false;
	}
	for ( size_t i = 0; i < prefix_size; i++ ) {
		if ( this->mBuffer [ i ] != prefix [ i ] ) {
			return false;
		}
	}
	return true;
}

bool String::StartsWith ( const wchar_t *prefix ) const {
	size_t prefix_size = wcslen ( prefix );
	if ( this->mSize < prefix_size ) {
		return false;
	}
	for ( size_t i = 0; i < prefix_size; i++ ) {
		if ( this->mBuffer [ i ] != prefix [ i ] ) {
			return false;
		}
	}
	return true;
}

bool String::StartsWith ( const String& prefix ) const {
	if ( this->mSize < prefix.mSize ) {
		return false;
	}
	for ( size_t i = 0; i < prefix.mSize; i++ ) {
		if ( this->mBuffer [ i ] != prefix.mBuffer [ i ] ) {
			return false;
		}
	}
	return true;
}

bool String::EndsWith ( const char c ) const {
	return this->Back ( ) == ( wchar_t ) c;
}

bool String::EndsWith ( const wchar_t c ) const {
	return this->Back ( ) == c;
}

bool String::EndsWith ( const char *suffix ) const {
	size_t suffx_size = strlen ( suffix );
	if ( this->mSize < suffx_size ) {
		return false;
	}
	const ptrdiff_t offset = this->mSize - suffx_size;
	for ( size_t i = 0; i < suffx_size; i++ ) {
		if ( this->mBuffer [ offset + i ] != ( wchar_t ) suffix [ i ] ) {
			return false;
		}
	}
	return true;
}

bool String::EndsWith ( const wchar_t *suffix ) const {
	size_t suffx_size = wcslen ( suffix );
	if ( this->mSize < suffx_size ) {
		return false;
	}
	const ptrdiff_t offset = this->mSize - suffx_size;
	for ( size_t i = 0; i < suffx_size; i++ ) {
		if ( this->mBuffer [ offset + i ] != suffix [ i ] ) {
			return false;
		}
	}
	return true;
}

bool String::EndsWith ( const String &suffix ) const {
	if ( this->mSize < suffix.mSize ) {
		return false;
	}
	const ptrdiff_t offset = this->mSize - suffix.mSize;
	for ( size_t i = 0; i < suffix.mSize; i++ ) {
		if ( this->mBuffer [ offset + i ] != suffix.mBuffer [ i ] ) {
			return false;
		}
	}
	return true;
}

size_t String::Find ( const char c ) const {
	for ( size_t i = 0; i < this->mSize; i++ ) {
		if ( wchar_t ( c ) == this->mBuffer [ i ] ) {
			return i;
		}
	}
	return String::npos;
}

size_t String::Find ( const wchar_t c ) const {
	for ( size_t i = 0; i < this->mSize; i++ ) {
		if ( c == this->mBuffer [ i ] ) {
			return i;
		}
	}
	return String::npos;
}

void String::Append ( const char *string ) {
	Append ( string , -1 );
}

void String::Append ( const char *string , size_t n ) {
	for ( size_t i = 0; i < n && string [ i ]; i++ ) {
		EnsureSpaceForOne ( );
		this->mBuffer [ this->mSize++ ] = ( wchar_t ) string [ i ];
	}
	this->mBuffer [ this->mSize ] = 0;
}

void String::AppendFormated ( const char *fmt , ... ) {
	va_list args;
	va_start ( args , fmt );
	AppendFormatedArgs ( fmt , args );
	va_end ( args );
}

void String::AppendFormatedArgs ( const char *fmt , va_list args ) {
	size_t length = 127;
	char *buffer = ( char * ) MEM_callocN ( length , __FUNCTION__ );
	do {
		length <<= 1;
		char *tmp = ( char * ) MEM_reallocN ( buffer , length + 1 );
		if ( !tmp ) {
			MEM_SAFE_FREE ( buffer );
		} else {
			buffer = tmp;
			buffer [ length ] = 0;
		}
	} while ( buffer && _vsnprintf ( buffer , length , fmt , args ) <= 0 );

	if ( buffer ) {
		UTF16_ENCODE ( buffer );
		this->Append ( buffer_16 );
		UTF16_UN_ENCODE ( buffer );
		MEM_SAFE_FREE ( buffer );
	} else {
		fprintf ( stderr , "%s failed to allocate enough memory.\n" , __FUNCTION__ );
	}
}

void String::Append ( const char c ) {
	EnsureSpaceForOne ( );
	this->mBuffer [ this->mSize++ ] = ( wchar_t ) c;
	this->mBuffer [ this->mSize ] = 0;
}

void String::Append ( const wchar_t *string ) {
	Append ( string , -1 );
}

void String::Append ( const wchar_t *string , size_t n ) {
	for ( size_t i = 0; i < n && string [ i ]; i++ ) {
		EnsureSpaceForOne ( );
		this->mBuffer [ this->mSize++ ] = ( wchar_t ) string [ i ];
	}
	this->mBuffer [ this->mSize ] = 0;
}

void String::AppendFormated ( const wchar_t *fmt , ... ) {
	va_list args;
	va_start ( args , fmt );
	AppendFormatedArgs ( fmt , args );
	va_end ( args );
}

void String::AppendFormatedArgs ( const wchar_t *fmt , va_list args ) {
	size_t length = 127;
	wchar_t *buffer = ( wchar_t * ) MEM_mallocN ( sizeof ( wchar_t ) * length , __FUNCTION__ );
	do {
		length <<= 1;
		wchar_t *tmp = ( wchar_t * ) realloc ( buffer , sizeof ( wchar_t ) * ( length + 1 ) );
		if ( !tmp ) {
			MEM_SAFE_FREE ( buffer );
		} else {
			buffer = tmp;
			buffer [ length ] = 0;
		}
	} while ( buffer && _vsnwprintf ( buffer , length , fmt , args ) <= 0 );

	if ( buffer ) {
		this->Append ( buffer );
	} else {
		fprintf ( stderr , "%s failed to allocate enough memory.\n" , __FUNCTION__ );
	}
}

void String::Append ( const wchar_t c ) {
	EnsureSpaceForOne ( );
	this->mBuffer [ this->mSize++ ] = c;
	this->mBuffer [ this->mSize ] = 0;
}

void String::Append ( const String &string ) {
	this->Append ( string.Data ( ) );
}

String String::SubStr ( size_t offset , size_t length ) const {
	return String ( this->Data ( ) + offset , length );
}

void String::Reserve ( size_t length ) {
	size_t new_alloc = ROSE_MAX ( ( size_t ) pow ( 2 , ceil ( log2 ( length + 1 ) ) ) , 1 );
	if ( this->mAllocated != new_alloc ) {
		wchar_t *tmp = ( wchar_t * ) MEM_reallocN ( this->mBuffer , new_alloc * sizeof ( wchar_t ) );
		if ( tmp ) {
			this->mBuffer = tmp;
			this->mAllocated = new_alloc;
		} else {
			fprintf ( stderr , "%s failed to allocate enough memory.\n" , __FUNCTION__ );
		}
	}
}

void String::Resize ( size_t length ) {
	this->Reserve ( length );
	this->mBuffer [ this->mSize = length ] = 0;
}

void String::EnsureSpaceForOne ( void ) {
	this->Reserve ( this->mSize + 1 );
}

bool operator < ( const String &a , const String &b ) { return wcscmp ( a.Data ( ) , b.Data ( ) ) < 0; }
bool operator <= ( const String &a , const String &b ) { return wcscmp ( a.Data ( ) , b.Data ( ) ) <= 0; }
bool operator == ( const String &a , const String &b ) { return wcscmp ( a.Data ( ) , b.Data ( ) ) == 0; }
bool operator != ( const String &a , const String &b ) { return wcscmp ( a.Data ( ) , b.Data ( ) ) != 0; }
bool operator >= ( const String &a , const String &b ) { return wcscmp ( a.Data ( ) , b.Data ( ) ) >= 0; }
bool operator > ( const String &a , const String &b ) { return wcscmp ( a.Data ( ) , b.Data ( ) ) > 0; }

String operator + ( const String &a , const String &b ) {
	return String ( { a , b } );
}

String &String::operator += ( const char *string ) {
	this->Append ( string );
	return *this;
}

String &String::operator += ( const char c ) {
	this->Append ( c );
	return *this;
}

String &String::operator += ( const wchar_t *string ) {
	this->Append ( string );
	return *this;
}

String &String::operator += ( const wchar_t c ) {
	this->Append ( c );
	return *this;
}

String &String::operator += ( const String &string ) {
	this->Append ( string );
	return *this;
}
