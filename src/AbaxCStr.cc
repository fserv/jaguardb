/*
 * Copyright JaguarDB
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <AbaxCStr.h>
#include <JagUtil.h>

AbaxCStr::AbaxCStr()
{
	_readOnly = false;
	initMem(1);
	memset( dtype, 0, 4 );
}

AbaxCStr::AbaxCStr( ssize_t size )
{
	_readOnly = false;
	initMem(size);
	memset( dtype, 0, 4 );
}

AbaxCStr::AbaxCStr(const char* str)
{
	_readOnly = false;
	if ( 0 == str ) {
		initMem(0);
	    return;
	}

	int len = strlen(str);
	initMem(len);
	length_ = len;
	memcpy(buf_, str, len );
	memset( dtype, 0, 4 );
	buf_[length_] = '\0';
}

AbaxCStr::AbaxCStr(const char* str, ssize_t capacity )
{
	_readOnly = false;
	if ( 0 == str ) {
		initMem(0);
	    return;
	}

	initMem(capacity);
    memset( buf_, 0, capacity+1);
    unsigned int slen = strlen(str);
    if ( slen > capacity ) {
        memcpy( buf_, str, capacity );
    } else {
        memcpy( buf_, str, slen );
    }
	length_ = capacity;
	memset( dtype, 0, 4 );
}

AbaxCStr::AbaxCStr(const char* str, ssize_t slen, ssize_t capacity )
{
	_readOnly = false;
	if ( 0 == str ) {
		initMem(0);
	    return;
	}

	initMem(capacity);
    memset( buf_, 0, capacity+1);
    if ( slen > capacity ) {
        memcpy( buf_, str, capacity );
    } else {
        memcpy( buf_, str, slen );
    }
	length_ = capacity;
	memset( dtype, 0, 4 );

}


AbaxCStr::AbaxCStr(const AbaxCStr& str)
{
	_readOnly = false;
	initMem( str.length_ );

	length_ = str.length_;
	memcpy(buf_, str.buf_, length_ );
	buf_[length_] = '\0';
	memcpy( dtype, str.dtype, 3 );
}

AbaxCStr::AbaxCStr(const char* str, const char *readOnly )
{
	if ( readOnly && strcmp(readOnly, "RDO")==0 ) {
		_readOnly = true;
	} else {
        abort();
    }

	buf_ = (char*)str;
	length_ = strlen(str);
}

AbaxCStr::AbaxCStr(const char* str, ssize_t strlen, const char *readOnly )
{
	if ( readOnly && strcmp(readOnly, "RDO")==0 ) {
		_readOnly = true;
	} else {
        abort();
    }

	buf_ = (char*)str;
	length_ = strlen;
}

AbaxCStr::~AbaxCStr()
{
	if ( ! _readOnly ) {
		if ( buf_ ) free(buf_);
	}
}

AbaxCStr& AbaxCStr::operator=( const AbaxCStr& s) 
{
	if ( this == &s ) {
		return *this;
	}

	if ( length_ < s.length_ ) {
		length_ = 0;
		allocMoreMemory( s.length_ ); 
	}

	length_ = s.length_;
	memcpy(buf_, s.buf_, length_);
	buf_[length_]= '\0';
	memcpy( dtype, s.dtype, 3 );

	return *this;
}

AbaxCStr& AbaxCStr::operator=(int c)
{
	length_ = 1;
	buf_[0] = c;
	buf_[1]= '\0';
	return *this;
}


AbaxCStr& AbaxCStr::operator+= ( const AbaxCStr &str )
{
	if ( _readOnly ) {
		printf(("s223820 error AbaxCStr::+= called on readOnly string\n"));
		exit(49);
	}

	if ( str.length() == 0 ) {
		return *this;
	}

	allocMoreMemory(str.length_ );
	memcpy(buf_+length_, str.buf_, str.length_);
	length_ += str.length_;
	buf_[length_] = '\0';
	return *this;
}

AbaxCStr& AbaxCStr::operator+=( const char *s)
{
	if ( _readOnly ) {
		printf(("s22420 error AbaxCStr::+= called on readOnly string\n"));
		exit(40);
	}

	int len2 = strlen(s);
	if ( s == NULL || 0 == len2 ) {
		return *this;
	}

	allocMoreMemory( len2);
	memcpy(buf_+length_, s, len2 );
	length_ += len2;
	buf_[length_] = '\0';
	return *this;
	return *this;
}

AbaxCStr&  AbaxCStr::operator+=( int ch )
{
	if ( _readOnly ) {
		printf(("s224920 error AbaxCStr::+= called on readOnly string\n"));
		exit(50);
	}

	allocMoreMemory(1);  
	buf_[length_] = ch;
	length_ += 1;
	buf_[length_] = '\0';
	return *this;
}

AbaxCStr&  AbaxCStr::operator+( int ch )
{
	if ( _readOnly ) {
		printf(("s254920 error AbaxCStr::+= called on readOnly string\n"));
		exit(50);
	}

	allocMoreMemory(1);  
	buf_[length_] = ch;
	length_ += 1;
	buf_[length_] = '\0';
	return *this;
}

AbaxCStr&  AbaxCStr::appendChars( int N, int ch )
{
	if ( _readOnly ) {
		printf(("s224920 error AbaxCStr::+= called on readOnly string\n"));
		exit(50);
	}

	allocMoreMemory(N);  

    memset( (void*)(buf_ + length_), ch, N );
    length_ += N;
	buf_[length_] = '\0';

	return *this;
}


AbaxCStr& AbaxCStr::append( const char *s, unsigned long len2)
{
	if ( _readOnly ) {
		printf(("s224920 error AbaxCStr::append called on readOnly string\n"));
		exit(51);
	}

	if ( s == NULL || 0 == len2 ) {
		return *this;
	}

	allocMoreMemory( len2);
	memcpy(buf_+length_, s, len2 );
	length_ += len2;
	buf_[length_] = '\0';
	return *this;
}


bool AbaxCStr::operator==( const char *s) const
{
	if ( NULL == s && buf_[0] != '\0' ) {
		return false;
	}

	if ( NULL == s && buf_[0] == '\0' ) {
		return true;
	}

	if ( 0 == strcmp(buf_, s) ) {
		return true;
	} else {
		return false;
	}
}

bool AbaxCStr::operator==( const AbaxCStr &s) const
{
	if ( 0 == strcmp(buf_, s.data() ) ) {
		return 1;
	} else {
		return 0;
	}
}

int AbaxCStr::compare( const AbaxCStr &s ) const
{
	return strcmp(buf_, s.data() );
}


bool AbaxCStr::operator<( const AbaxCStr &s ) const
{
	int rc = strcmp(buf_, s.data() );
	if ( rc < 0 ) return 1;
	else return 0;
}
bool AbaxCStr::operator<=( const AbaxCStr &s ) const
{
	int rc = strcmp(buf_, s.data() );
	if ( rc <= 0 ) return 1;
	else return 0;
}
bool AbaxCStr::operator>( const AbaxCStr &s ) const
{
	int rc = strcmp(buf_, s.data() );
	if ( rc > 0  ) { return 1; } else { return 0; }
}
bool AbaxCStr::operator>=( const AbaxCStr &s ) const
{
	int rc = strcmp(buf_, s.data() );
	if ( rc >= 0  ) { return 1; } else { return 0; }
}

bool AbaxCStr::operator!=( const char *s) const
{
	if ( 0 == strcmp(buf_, s) ) {
		return 0;
	} else {
		return 1;
	}
}

bool AbaxCStr::operator!=( const AbaxCStr &s) const
{
	if ( 0 == strcmp(buf_, s.data() ) ) {
		return 0;
	} else {
		return 1;
	}
}

int AbaxCStr::toInt() const
{
	return atoi(buf_);
}

int AbaxCStr::toi() const
{
	return atoi(buf_);
}

unsigned short AbaxCStr::toUshort() const
{
	return (unsigned short)(atoi(buf_));
}

long  AbaxCStr::toLong() const
{
	return strtol(buf_, (char**)NULL, 10 );
}

long  AbaxCStr::tol() const
{
	return strtol(buf_, (char**)NULL, 10 );
}

long double  AbaxCStr::toLongDouble() const
{
	return strtold(buf_, (char**)NULL );
}

long double  AbaxCStr::told() const
{
	return strtold(buf_, (char**)NULL );
}

unsigned long  AbaxCStr::toULong() const
{
	return strtoul(buf_, (char**)NULL, 10 );
}

unsigned long  AbaxCStr::toul() const
{
	return strtoul(buf_, (char**)NULL, 10 );
}

long long  AbaxCStr::toLLong() const
{
	if ( NULL == buf_ || '\0' == *buf_ ) return 0;
    return atoll( buf_ );
}

// end=1: trim at end only; end=2: trim at both ends
AbaxCStr& AbaxCStr::trimSpaces( int end )
{
	if ( _readOnly ) {
		printf(("s21920 error AbaxCStr::trimSpaces called on readOnly string\n"));
		exit(52);
	}

	if ( length_ < 1 ) return *this;

	char c;
	int i;
	for( i=length_-1; i>=0; i--) {
		c = buf_[i];
		if ( c == ' ' || c == '\t' || c == '\r' || c == '\n' ) {
			buf_[i]='\0';
		} else {
			break;
		}
	}

	length_ = i+1;
	if ( end == 1 ) {
		return *this;
	}

	char *pstart=0;
	int  nbeg=0;
	for( char *p=buf_; *p != '\0'; p++) {
		if ( *p == ' ' || *p == '\t' || *p == '\r' || *p == '\n' ) {
			nbeg ++;
		} else {
			pstart = p;
			break;
		}
	}

	length_ = length_ - nbeg;
	AbaxCStr *ps = new AbaxCStr(length_ );
	memcpy(ps->buf_, pstart, length_);

	if ( buf_ ) free(buf_);
	buf_ = ps->buf_;
	buf_[length_] = '\0';
	nseg_ = ps->nseg_;
	return *this;
}

void AbaxCStr::trimNull()
{
	if ( _readOnly ) {
		printf(("s219400 error AbaxCStr::trimNull called on readOnly string\n"));
		exit(53);
	}

	for ( int i = length_-1; i>=0; --i ) {
		if ( buf_[i] == '\0' ) { --length_; }
	}
}

AbaxCStr& AbaxCStr::trimChar( char C )
{
	if ( _readOnly ) {
		printf(("s219430 error AbaxCStr::trimChar called on readOnly string\n"));
		exit(54);
	}

	if ( length_ < 1 ) return *this;

	int i = length_-1;
	if ( C == buf_[i] ) {
		buf_[i]='\0';
		--length_;
	}

	if ( buf_[0] != C ) {
		return *this;
	}

	--length_; 
	AbaxCStr *ps = new AbaxCStr(length_+1 );
	memcpy(ps->buf_, buf_+1, length_);

	if ( buf_ ) free(buf_);
	buf_ = ps->buf_;
	buf_[length_] = '\0';
	nseg_ = ps->nseg_;
	return *this;
}


AbaxCStr & AbaxCStr::trimEndChar( char chr )
{
	if ( _readOnly ) {
		printf(("s219436 error AbaxCStr::trimEndChar called on readOnly string\n"));
		exit(55);
	}

	if ( length_ < 1 ) return *this;
	char c;
	int i;
	for( i=length_-1; i>=0; i--) {
		c = buf_[i];
		if ( c == chr || c== ' ' || c == '\t' || c == '\r' || c == '\n' ) {
			buf_[i]='\0';
		} else {
			break;
		}
	}
	length_ = i+1;
	return *this;
}

void AbaxCStr::remove( char c )
{
	if ( _readOnly ) {
		printf(("s219436 error AbaxCStr::remove called on readOnly string\n"));
		exit(56);
	}

	if ( !strchr(buf_, c) ) return;
	AbaxCStr *newStr = new AbaxCStr();
	int nlen = 0;
	for (char *p=buf_; *p != '\0'; p++) {
		if ( *p == c ) { }
		else {
			*newStr += *p;
			nlen ++;
		}
	}

	if ( buf_ ) free(buf_);
	buf_ = newStr->buf_;
	nseg_ = newStr->nseg_;
	length_ = nlen;
}

void AbaxCStr::remove( const char *chset )
{
	if ( _readOnly ) {
		printf(("s259436 error AbaxCStr::remove called on readOnly string\n"));
		exit(57);
	}

	AbaxCStr *newStr = new AbaxCStr();
	int nlen = 0;
	char c;
	for ( int i=0; i < length_; ++i ) {
		c = buf_[i];
		if ( strchr( chset, c ) ) {
			// left out
		} else {
			*newStr += c;
			nlen ++;
		}
	}

	if ( buf_ ) free(buf_);
	buf_ = newStr->buf_;
	nseg_ = newStr->nseg_;
	length_ = nlen;
}


void AbaxCStr::replace( char old, char newc )
{
	if ( _readOnly ) {
		printf(("s255436 error AbaxCStr::replace called on readOnly string\n"));
		exit(58);
	}

	for ( int i=0; i < length_; ++i ) {
		if ( buf_[i] == old ) {
			buf_[i] = newc;
		}
	}
}

void AbaxCStr::replace( const char *chset, char newc )
{
	if ( _readOnly ) {
		printf(("s205336 error AbaxCStr::replace called on readOnly string\n"));
		exit(59);
	}

	for (char *old=(char*) chset; *old != '\0'; old++) {
		for (char *p=buf_; *p != '\0'; p++) {
			if ( *p == *old ) {
				*p = newc;
			}
		}
	}
}

void AbaxCStr::removeString( const char *oldstr ) 
{
	replace( oldstr, "" ); 
}

void AbaxCStr::replace( const char *oldstr, const char *newstr)
{
	if ( _readOnly ) {
		printf(("s105336 error AbaxCStr::replace called on readOnly string\n"));
		exit(60);
	}

	AbaxCStr *pnew = new AbaxCStr();
	int  olen = strlen(oldstr);

	char *p = buf_;
	while ( *p != '\0' ) {
		if ( 0 == strncmp(p, oldstr, olen) ) {
			*pnew += newstr;
			p += olen;
		} else {
			*pnew += *p++;
		}
	}

	length_ = pnew->length();
	if ( buf_ ) free(buf_);
	buf_ = pnew->buf_;
	nseg_ = pnew->nseg_;
}

void AbaxCStr::initMem( int size )
{
	nseg_ = int(size/ASTRSIZ) + 1;
	buf_= (char*)malloc(nseg_ * ASTRSIZ);
	memset( (void*)buf_, 0, nseg_ * ASTRSIZ );
	length_=0;
}

void AbaxCStr::allocMoreMemory( int len2 )
{
	int newLen = length_ + len2;
	int newSegs = int( newLen/ASTRSIZ ) + 1;
	if ( newSegs > nseg_ ) {
		buf_ = (char*)realloc( buf_, newSegs * ASTRSIZ );
		buf_[length_] = '\0';
		nseg_ = newSegs;
	}
}

int AbaxCStr::countChars( char c) const
{
	int n=0;
	for ( int i=0; i < length_; ++i ) {
		if ( buf_[i] == c ) ++n;
	}
	return n;
}

AbaxCStr AbaxCStr::substr(int start, int len ) const
{
	return AbaxCStr(buf_+start, len );
}

AbaxCStr AbaxCStr::substr(int start ) const
{
	return AbaxCStr(buf_+start, length_ - start );
}

AbaxCStr & AbaxCStr::pad0()
{
	if ( _readOnly ) {
		printf(("s105836 error AbaxCStr::pad0 called on readOnly string\n"));
		exit(61);
	}

	if ( length_ == 1 )
	{
		int len = length_;
		char *pc = (char*)malloc(len + 1);
		memcpy(pc, buf_, len);

		allocMoreMemory(1);

		buf_[0] = '0';
		memcpy( buf_+1, pc, len); 
		free(pc);
		buf_[length_] = '\0';
	}

	return *this;
}

void AbaxCStr::toUpper() 
{
	for(int i=0; i< length_; i++) {
		buf_[i] = toupper( buf_[i] );
	}
}

void AbaxCStr::toLower()
{
	for(int i=0; i< length_; i++) {
		buf_[i] = tolower( buf_[i] );
	}
}

int AbaxCStr::operator[] (int i) const
{
	if ( i<length_ ) {
		return buf_[i];
	} else {
		return 0;
	}
}

AbaxCStr AbaxCStr::operator+(const AbaxCStr &s2 ) const
{
	if ( _readOnly ) {
		printf(("s105833 error AbaxCStr::+ called on readOnly string\n"));
		exit(62);
	}

	char *buf = (char*)malloc( length_ + s2.length_ + 1 );
	memcpy(buf, buf_, length_ );
	memcpy(buf+length_, s2.buf_, s2.length_ );
	buf[length_ + s2.length_] = '\0';
	AbaxCStr res(buf, length_ + s2.length_ );
	free( buf );
	return res;
}

AbaxCStr operator+(const char *s1, const AbaxCStr &s2 )
{
	AbaxCStr res = s1;
	res += s2;
	return res;
}


int AbaxCStr::caseEqual(const char *str) const
{
	int ilen = strlen(str);
	if ( ilen != length_ ) return 0;

	for ( int i=0; i<length_; i++) {
		if ( toupper(buf_[i]) != toupper( str[i] ) ) {
			return 0;
		}
	}
	return 1;
}

int AbaxCStr::numPunct() const
{
	int c = 0;
	for ( int i=0; i<length_; i++) {
		if ( ::ispunct(buf_[i]) )  ++c;
	}
	return c;
}

ssize_t AbaxCStr::find( int c) const
{
	char *p = strchr( buf_ , c );
	if ( ! p ) return -1;
	return ( p - buf_ );
}

void AbaxCStr::print() const
{
	printf("AbaxCStr::print() length_=%ld:\n[", length_ );
	for ( int i=0; i < length_; ++i ) {
		if ( buf_[i] == '\0' ) {
			putchar( '@' );
		} else {
			putchar( buf_[i] );
		}
	}
	putchar(']');
	putchar('\n');
	putchar('\n');
}

#if 0
AbaxCStr&  AbaxCStr::trimEndZeros()
{
	if ( _readOnly ) {
		printf(("s145833 error AbaxCStr::trimEndZeros called on readOnly string\n"));
		exit(63);
	}

	if ( length_ < 1 ) return *this;
	if ( ! strchr( buf_, '.') ) return *this;
	if ( ! buf_ ) return *this;

    if ( '0' != buf_[length_ - 1] ) {
        return *this;
    }

    if ( '.' == buf_[length_ - 1] ) {
        --length_;
        return *this;
    }

	char *buf= (char*)malloc(nseg_ * ASTRSIZ);
	memset( (void*)buf, 0, nseg_ * ASTRSIZ );

    int start=0;
    bool leadzero = false;
    int len = 0;
    if ( buf_[0] == '+' || buf_[0] == '-' ) {
		buf[len++] = buf_[0];
        start = 1;
    }

	if ( buf_[start] == '0' && buf_[start+1] != '\0' ) leadzero = true;
	for ( int i = start; i < length_; ++i ) {
		if ( buf_[i] != '0' || ( buf_[i] == '0' && buf_[i+1] == '.' ) ) {
			leadzero = false;
		}
		if ( ! leadzero ) {
			buf[len++] = buf_[i];
		}
	}

    if ( len < 1 ) {
		buf[0] = '0'; buf[1] = '\0'; len=1;
		free( buf_ );
		buf_ = buf;
		length_ = len;
		return *this;
	}

	char *p = buf+len-1;
	while ( p >= buf+1 ) {
		if ( *(p-1) != '.' && *p == '0' ) {
			*p = '\0';
			--len;
		} else {
			break;
		}
		--p;
	}

	if ( buf[0] == '.' && buf[1] == '\0' ) { buf[0] = '0'; len=1; }
	else if ( buf[0] == '.' && buf[1] == '0' ) { buf[0] = '0'; len=1; }
	else if ( buf[0] == '\0' || len==0 ) {  buf[0] = '0'; buf[1] = '\0'; len=1; }
    else {
        if ( len >= 3 ) {
            if ( buf[len-2] == '.' && buf[len-1] == '0' ) {
                len -= 2;
            } else if ( buf[len-1] == '.' ) {
                --len; 
            }
        }
    }

	free( buf_ );
	buf_ = buf;
	length_ = len;
	return *this;
}
#endif

AbaxCStr&  AbaxCStr::trimEndZeros()
{
	if ( _readOnly ) {
		printf(("s145833 error AbaxCStr::trimEndZeros called on readOnly string\n"));
		exit(63);
	}

	if ( length_ < 1 ) return *this;
	if ( ! strchr( buf_, '.') ) {
        if ( isAllZero() ) {
            buf_[0] = '0';
            buf_[1] = '\0';
            length_ = 1;
        }

        return *this;
    }

	if ( ! buf_ ) return *this;

    if ( length_ >= 2 && '.' == buf_[length_ - 1] ) {
        buf_ [ length_ - 1] = '\0';
        --length_;
        return *this;
    }

    if ( '0' != buf_[length_ - 1] ) {
        return *this;
    }

    for ( int i = length_ - 1 ; i >=0; --i ) { 
        if ( buf_[i] == '.' ) {
            buf_[i] = '\0';
            -- length_;
            break;
        }

        if ( buf_[i] == '0' ) {
            buf_[i] = '\0';
            -- length_;
        } else {
            break;
        }
    }

    if ( buf_[length_ -1 ] == '.' ) {
        buf_[length_ -1] = '\0';
        --length_;
    }

    if ( 0 == length_ ) {
          buf_[0] = '0';
          length_ = 1;
    } 

	return *this;
}


AbaxCStr AbaxCStr::firstToken( char sep )
{
	if ( length_ < 1 ) return "";
	char *p = buf_;
	while ( *p != sep && *p != '\0' ) ++p;
	return AbaxCStr(buf_, p-buf_);
}

const char * AbaxCStr::secondTokenStart( char sep )
{
	if ( length_ < 1 ) return NULL;
	char *p = buf_;
	while ( *p != sep && *p != '\0' ) ++p;
	if ( *p == '\0' ) return NULL;
	while ( *p == sep ) ++p;
	return p;
}

AbaxCStr AbaxCStr::substrc( char startc, char endc ) const
{
	const char *p = buf_;
	while ( *p != startc && *p != '\0' ) ++p;
	if ( *p == '\0' ) return "";
	++p;
	if ( *p == '\0' ) return "";

	const char *q = p;
	while ( *q != endc && *q != '\0' ) ++q;
	if ( *q == '\0' ) return p;  
	return AbaxCStr(p, q-p);
}

bool  AbaxCStr::isNull() const
{
	if ( buf_[0] == '\0' ) return true;
	return false;
}

bool  AbaxCStr::isNotNull() const
{
	if ( buf_[0] != '\0' ) return true;
	return false;
}

bool AbaxCStr::containsChar( char c ) const
{
	if ( buf_[0] == '\0' ) return false;
	if ( strchr( buf_, c ) ) return true;
	return false;
}

bool AbaxCStr::containsStr( const char *substr ) const
{
	if ( buf_[0] == '\0' ) return false;
	if ( strstr( buf_, substr) ) return true;
	return false;
}

bool AbaxCStr::containsStrCase( const char *substr, AbaxCStr &ret ) const
{
	if ( buf_[0] == '\0' ) return false;
	char *p = strcasestr( buf_, substr);
	if ( p ) {
		ret = AbaxCStr(p, strlen(substr) );
		return true;
	}
	return false;
}

double AbaxCStr::tof() const
{
	if ( ! buf_ || buf_[0] == '\0' ) return 0.0;
	return atof( buf_ );
}

double AbaxCStr::tod() const
{
    return tof();
}

bool AbaxCStr::isNumeric() const
{
	for ( int i=0; i < length_; ++i ) {
		if ( ! isdigit(buf_[i]) && buf_[i] != '.' ) {
			return false;
		}
	}
	return true;
}

bool AbaxCStr::isDigit() const
{
	for ( int i=0; i < length_; ++i ) {
		if ( ! isdigit(buf_[i]) ) {
			return false;
		}
	}
	return true;
}

bool AbaxCStr::isAllZero() const
{
	for ( int i=0; i < length_; ++i ) {
		if ( buf_[i] != '0' ) {
			return false;
		}
	}
	return true;
}

void AbaxCStr::dump()
{
	for ( int i=0; i < length_; ++i ) {
		if ( buf_[i] == '\0' ) {
			printf("@");
		} else {
			printf("%c", buf_[i] );
		}
	}
	printf("\n");
	fflush( stdout );
}

void AbaxCStr::setDtype( const char *typ )
{
	if ( _readOnly ) {
		printf(("s145853 error AbaxCStr::setDtype called on readOnly string\n"));
		exit(64);
	}

	int len = strlen(typ);
	if ( len > 3 ) { len = 3; }
	memcpy( dtype, typ, len );
}

char  AbaxCStr::firstChar() const
{
	return buf_[0];
}

char  AbaxCStr::lastChar() const
{
	return buf_[length_-1];
}

AbaxCStr  AbaxCStr::lastCharStr() const
{
	char buf[2];
	buf[0] =  buf_[length_-1];
	buf[1] = '\0';
	return AbaxCStr( buf );
}

// compare self and str, skipping any char in skips
bool AbaxCStr::caseMatch( const char *str, const char *skips)
{
	char c1;
	const char *p2 = str;
	for ( int i=0; i < length_; ++i ) {

		if ( *p2 == '\0' ) {
			break;
		}

		c1 = buf_[i];
		if ( strchr(str, c1) ) {
			continue;
		}

		if ( strchr(str, *p2) ) {
			++p2;
			continue;
		}
		
		if ( toupper(c1) != toupper(*p2) ) {
			return false;
		}

		++p2;
	}	

	return true;
}


// words: "key2 ff3  gg5"
// must contain all words inside words
// skips are delimiters
bool AbaxCStr::containAllWords( const char *cwords, const char *skips, bool ignoreCase )
{
	char *save;

	if ( NULL == cwords || *cwords == '\0' ) return false;

	char *words = strdup(cwords);

	char *tok = strtok_r( words, skips, &save);
	if ( NULL == tok ) {
		free(words);
		return false;
	}

	char *pe;
	if ( ignoreCase ) {
		pe = strcasestr(buf_, tok);
	} else {
		pe = strstr(buf_, tok);
	}

	if ( ! pe ) {
		free(words);
		return false;
	}

	while ( NULL != (tok = strtok_r(NULL, skips, &save)) ) {
		if ( ignoreCase ) {
			pe = strcasestr(buf_, tok);
		} else {
			pe = strstr(buf_, tok);
		}

		if ( ! pe ) {
			free(words);
			return false;
		}
	}

	free(words);
	return true;
}


// words: "key2 ff3  gg5"
// any contain any word inside words
// skips are delimiters
bool AbaxCStr::containAnyWord( const char *cwords, const char *skips, bool ignoreCase )
{
	char *save;

	if ( NULL == cwords || *cwords == '\0' ) return false;

	char *words = strdup(cwords);

	char *tok = strtok_r( words, skips, &save);
	if ( NULL == tok ) {
		free(words);
		return false;
	}

	char *pe;
	if ( ignoreCase ) {
		pe = strcasestr(buf_, tok);
	} else {
		pe = strstr(buf_, tok);
	}

	if ( pe ) {
		free(words);
		return true;
	}

	while ( NULL != (tok = strtok_r(NULL, skips, &save)) ) {
		if ( ignoreCase ) {
			pe = strcasestr(buf_, tok);
		} else {
			pe = strstr(buf_, tok);
		}

		if ( pe ) {
			free(words);
			return true;
		}
	}

	free(words);
	return false;
}

void MurmurHash3_x64_128 ( const void * key, const int len, const unsigned int seed, void * out );

size_t AbaxCStr::hashCode() const
{
    unsigned int hash[4];
    unsigned int seed = 42;
    MurmurHash3_x64_128( (void*)buf_, length_, seed, hash);
    uint64_t res = ((uint64_t*)hash)[0];
    return  res % JAG_LONG_MAX;
}

AbaxCStr AbaxCStr::condenseSpaces()
{
	int prev = -1;
	const char *p = buf_;
    AbaxCStr res;

	for ( int i=0; i < length_; ++i ) {
		if ( ! (*p == ' ' && prev == ' ' ) ) {
            res += *p;
        }

        prev = *p;
		++p;
	}

    return res;
}
