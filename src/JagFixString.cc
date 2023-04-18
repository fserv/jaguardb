/* * Copyright JaguarDB
 *
 * This file is part of JaguarDB.
 *
 * JaguarDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JaguarDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JaguarDB (LICENSE.txt). If not, see <http://www.gnu.org/licenses/>.
 */
#include <JagGlobalDef.h>

#include <abax.h>
#include <JagUtil.h>

JagFixString::JagFixString() 
{
	_buf = NULL;
	_length = 0;
	_readOnly = false;
	memset( dtype, 0, 4 );
}

JagFixString::JagFixString( const char *str ) 
{
	unsigned int len = strlen( str );
	_readOnly = false;

	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str, len );
	_buf[len] = '\0';
	_length = len;
	memset( dtype, 0, 4 );
}

JagFixString::JagFixString( const char *str, unsigned int len ) 
{ 
	_readOnly = false;

	/**
	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str, len );
	_buf[len] = '\0';
	_length = len;
	**/

	_buf = (char*)jagmalloc(len+1);
	memset( _buf, 0, len+1);
	unsigned int slen = strlen(str);
	if ( slen > len ) {
		memcpy( _buf, str, len );
	} else {
		memcpy( _buf, str, slen );
	}
	_length = len;
	memset( dtype, 0, 4 );
}


JagFixString::JagFixString( const char *str, unsigned int slen, unsigned int capacity ) 
{ 
	_readOnly = false;

	_buf = (char*)jagmalloc(capacity+1);
	memset( _buf, 0, capacity+1);
	if ( slen > capacity ) {
		memcpy( _buf, str, capacity );
	} else {
		memcpy( _buf, str, slen );
	}
	_length = capacity;
	memset( dtype, 0, 4 );
}

JagFixString::JagFixString( const JagFixString &str ) 
{ 
	_readOnly = false;
	int len = str._length;
	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str._buf, len );
	_buf[len] = '\0';
	_length = len;

	memcpy( dtype, str.dtype, 3 );
}
	
JagFixString::JagFixString( const Jstr &str ) 
{ 
	_readOnly = false;
	int len = str.size();
	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str.c_str(), len );
	_buf[len] = '\0';
	_length = len;
	memcpy( dtype, str.dtype, 3 );
}

JagFixString& JagFixString::operator=( const char *str ) 
{ 
	if ( _buf == str ) {
		return *this;
	}

	if ( _buf && ! _readOnly ) {
		free ( _buf );
	}

	int len = strlen(str);
	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str, len );
	_length = len;
	_buf[len] = '\0';
	_readOnly = false;
	memset( dtype, 0, 4 );
	return *this;
}

JagFixString& JagFixString::operator=( const JagFixString &str ) 
{ 
	if ( _buf == str._buf ) {
		return *this;
	}

	if ( _buf && ! _readOnly ) {
		free ( _buf );
	}

	int len = str._length;
	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str._buf, len );
	_length = len;
	_buf[len] = '\0';
	_readOnly = false;
	memcpy( dtype, str.dtype, 3 );
	return *this;
}
 		
JagFixString& JagFixString:: operator=( const Jstr &str ) 
{ 
	if ( _buf && ! _readOnly ) {
		free ( _buf );
	}

	int len = str.size();
	_buf = (char*)jagmalloc(len+1);
	memcpy( _buf, str.c_str(), len );
	_buf[len] = '\0';
	_length = len;
	_readOnly = false;
	memcpy( dtype, str.dtype, 3 );
	return *this;
}

int JagFixString::operator== ( const JagFixString &s2 )  const 
{
    return (memcmp(_buf, s2._buf, _length ) == 0);
}


int JagFixString::operator< ( const JagFixString &s2 ) const 
{
	if ( ! _buf && ! s2._buf ) return 0;
	if ( ! _buf ) return 1;
	if ( ! s2._buf ) return 0;
    return (memcmp(_buf, s2._buf, _length ) < 0);
}

int JagFixString::operator<= ( const JagFixString &s2 ) const 
{
	if ( ! _buf && ! s2._buf ) return 1;
	if ( ! _buf ) return 1;
	if ( ! s2._buf ) return 0;
    return (memcmp(_buf, s2._buf, _length ) <= 0);
}

int JagFixString::operator> ( const JagFixString &s2 ) const 
{
	if ( ! _buf && ! s2._buf ) return 0;
	if ( ! _buf ) return 0;
	if ( ! s2._buf ) return 1;
   	return (memcmp(_buf, s2._buf, _length ) > 0);
}

int JagFixString::operator>= ( const JagFixString &s2 ) const 
{
	if ( ! _buf && ! s2._buf ) return 1;
	if ( ! _buf ) return 0;
	if ( ! s2._buf ) return 1;
   	return (memcmp(_buf, s2._buf, _length ) >= 0);
}

JagFixString& JagFixString::operator+= (const JagFixString &s ) 
{
	_buf = (char*)realloc( (void*)_buf, _length+s._length+1 );
	memcpy( _buf+_length, s._buf, s._length );
	_buf[_length+s._length] = '\0';
	_length += s._length;
	_readOnly = false;
	return *this;
}

JagFixString JagFixString::operator+ (const JagFixString &s ) const 
{
	JagFixString res = *this;
	res += s;
	return res;
}

void JagFixString::point( const JagFixString &fs )
{
	point( fs.c_str(), fs.length() );
}

void JagFixString::point(const char *str, unsigned int len )
{
	if ( _buf && ! _readOnly ) {
		free( _buf );
	}

	_buf = (char*)str;
	_length = len;
	_readOnly = true;
}

void JagFixString::strcpy( const char *data )
{
	::strcpy(_buf, data );
}

JagFixString::~JagFixString()
{
	if ( _readOnly ) {
		return;
	}

	if ( _buf )
	{
		free ( _buf );
	}
	_buf = NULL;
}

jagint JagFixString::hashCode() const 
{
    unsigned int hash[4];                
    unsigned int seed = 42;             
	char *p;
	char *newbuf = (char*)jagmalloc( _length + 1 );
	memset( newbuf, 0, _length +1);
	int len = 0;
	p = newbuf;
	for ( int i =0; i < _length; ++i ) {
		if ( _buf[i] != '\0' ) {
			*p ++ = _buf[i];
			++len;
		}
	}

    MurmurHash3_x64_128( (void*)newbuf, len, seed, hash);
    uint64_t res2 = ((uint64_t*)hash)[0]; 
    jagint res = res2 % LLONG_MAX;
	free( newbuf );
    return res;
}


void JagFixString::ltrim()
{
	int i;
	char *p = _buf;
	if ( *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n' ) return;
	while ( *p == ' ' || *p == '\t' && *p == '\r' && *p == '\n' ) ++p;
	int d = p-_buf; 
	for ( i = d; i < _length; ++i ) {
		_buf[i-d] = _buf[i];
	}
	for ( i = _length-d; i < _length; ++i ) {
		_buf[i] = '\0';
	}
}

void JagFixString::rtrim()
{
	if ( _buf[0] == '\0' ) return;
	char *p = _buf + _length - 1;
	while ( *p == '\0' && p != _buf ) --p;
	if ( p == _buf ) return;
	while ( *p == ' ' || *p == '\t' && *p == '\r' && *p == '\n' ) { *p = '\0'; --p; }
}

void JagFixString::trim()
{
	ltrim();
	rtrim();
}

void JagFixString::substr( jagint start, jagint len )
{
	int i;
	if ( start < 0 ) start = 0;
	else if ( start >= _length ) start = _length-1;

	/**
	if ( len > (_length - start) ) {
		len = _length - start;
	}
	**/

	if ( len < 0 ) {
		/**
    	for ( i = start; i < _length; ++i ) {
    		_buf[i-start] = _buf[i];
    	}
    	for ( i = _length-start; i < _length; ++i ) {
    		_buf[i] = '\0';
    	}
		**/
	} else if ( len == 0 ) {
		memset( _buf, 0, _length );
	} else {
		if ( start + len > _length ) {
			len = _length - start;
		}

    	for ( i = start; i < start + len; ++i ) {
    		_buf[i-start] = _buf[i];
    	}
    	for ( i = len; i < _length; ++i ) {
    		_buf[i] = '\0';
    	}
	}
}

JagFixString JagFixString::concat( const JagFixString& s2 )
{
	int s2len = s2.length();
	char newbuf[_length + s2len + 1 ];
	char *p, *pb;
	pb = newbuf;
	for ( p = _buf; *p != '\0' && (p-_buf)<_length; ++p ) {
		*pb++ = *p;
	}

	for ( p = (char*)s2.c_str(); p < (char*)s2.c_str() + s2len; ++p ) {
		*pb++ = *p;
	}
	*pb = '\0';

	return JagFixString( newbuf, _length + s2len );
}


void JagFixString::print() const
{
	printf("Fixstr print():\n");
	for ( int i=0; i < _length; ++i ) {
		if ( _buf[i] ) {
			printf("%c", _buf[i] );
		} else {
			printf("@" );
		}
	}
	printf("\n"); fflush( stdout );
}

void JagFixString::replace( char oldc, char newc )
{
	for ( int i=0; i < _length; ++i ) {
		if ( oldc == _buf[i] ) {
			_buf[i] = newc;
		}
	}
}


Jstr JagFixString::firstToken( char sep ) const
{
    if ( _length < 1 ) return "";
    char *p = _buf;
    while ( *p != sep && *p != '\0' ) ++p;
    return AbaxCStr(_buf, p-_buf);
}

void JagFixString::setDtype( const char *typ )
{
    int len = strlen(typ);
    if ( len > 3 ) { len = 3; }
    memcpy( dtype, typ, len );
}

