#pragma once

#include "lib_typed_buffer.h"
#include "lib_allocator.h"
#include "lib_utildefines.h"
#include "lib_span.h"
#include "lib_vector.h"
#include "lib_string_util.h"

#include <sstream>
#include <string>

class String {
	wchar_t *mBuffer;
	size_t mAllocated;
	size_t mSize;
public:
	String ( );
	String ( const String &string );
	String ( char c );
	String ( wchar_t c );
	String ( const char *string );
	String ( const char *string , size_t len );
	String ( const wchar_t *string );
	String ( const wchar_t *string , size_t len );
	String ( std::initializer_list<String> list );
	~String ( );

	void Reserve ( size_t length );
	void Resize ( size_t length );

	void Append ( const char *string );
	void Append ( const char *string , size_t n );
	void AppendFormated ( const char *fmt , ... );
	void AppendFormatedArgs ( const char *fmt , va_list args );
	void Append ( const char c );

	void Append ( const wchar_t *string );
	void Append ( const wchar_t *string , size_t n );
	void AppendFormated ( const wchar_t *fmt , ... );
	void AppendFormatedArgs ( const wchar_t *fmt , va_list args );
	void Append ( const wchar_t c );

	void Append ( const String &string );

	String SubStr ( size_t offset , size_t length = -1 ) const;

	inline bool IsEmpty ( ) const { return this->mSize == 0; }

	// Returns the cached string size.
	inline size_t Size ( ) const { return this->mSize; }

	// Calls wcslen to find the size of the string.
	inline size_t Length ( ) const { return wcslen ( this->mBuffer ); }

	inline wchar_t *Data ( ) { return this->mBuffer; }
	inline const wchar_t *Data ( ) const { return this->mBuffer; }

	inline wchar_t *WCStr ( ) { return this->mBuffer; }
	inline const wchar_t *WCStr ( ) const { return this->mBuffer; }

	std::string ToString ( ) const {
		return std::string ( this->Data ( ) , this->Data ( ) + this->Size ( ) );
	}

	std::wstring ToWString ( ) const {
		return std::wstring ( this->Data ( ) , this->Data ( ) + this->Size ( ) );
	}

	inline wchar_t &operator [ ] ( size_t index ) { return this->mBuffer [ index ]; }
	inline const wchar_t &operator [ ] ( size_t index ) const { return this->mBuffer [ index ]; }

	String &operator += ( const char *string );
	String &operator += ( const char c );

	String &operator += ( const wchar_t *string );
	String &operator += ( const wchar_t c );

	String &operator += ( const String& string );
private:
	void EnsureSpaceForOne ( void );

	friend bool operator < ( const String &a , const String &b );
	friend bool operator <= ( const String &a , const String &b );
	friend bool operator == ( const String &a , const String &b );
	friend bool operator != ( const String &a , const String &b );
	friend bool operator >= ( const String &a , const String &b );
	friend bool operator > ( const String &a , const String &b );

	friend String operator + ( const String &a , const String &b );
};

bool operator < ( const String &a , const String &b );
bool operator <= ( const String &a , const String &b );
bool operator == ( const String &a , const String &b );
bool operator != ( const String &a , const String &b );
bool operator >= ( const String &a , const String &b );
bool operator > ( const String &a , const String &b );

String operator + ( const String &a , const String &b );
