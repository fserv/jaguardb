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
#ifndef _abax_cstr_h_
#define _abax_cstr_h_
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#ifdef USE_MALLOC
#include <malloc.h>
#else
#include <jemalloc.h>
#endif


class AbaxCStr
{
	public:   

        static pthread_mutex_t     strMutex;

		AbaxCStr(); 
		AbaxCStr(const AbaxCStr& str);
		AbaxCStr(const char* str);
		AbaxCStr(const char* str, ssize_t capacity );
		AbaxCStr(const char* str, ssize_t strlen, ssize_t capacity );
		AbaxCStr(ssize_t size);
		AbaxCStr(const char* str, const char *readOnly );
		AbaxCStr(const char* str, ssize_t strlen, const char *readOnly );
		~AbaxCStr();
		const char*     c_str() const { return buf_; }
		char*           start() const { return buf_; }
		const char*     addr() const { return buf_; }
		const char*     data() const { return buf_; }
		const char      *s() const { return buf_; }
		ssize_t     size() const { return length_; }
		ssize_t     length() const { return length_; }
		ssize_t     find( int c) const;
		double      tof() const;
		double      tod() const;
		int         toInt() const;
		int         toi() const;
		unsigned short toUshort() const;
		long         toLong() const;
		long         tol() const;
		long double  toLongDouble() const;
		long double  told() const;
		unsigned long  toULong() const;
		unsigned long  toul() const;
		long long   toLLong() const;
		char        firstChar() const;
		char        lastChar() const;
		AbaxCStr    lastCharStr() const;

		AbaxCStr& operator +=( const AbaxCStr & cpstr);
		AbaxCStr& operator +=( const char *cpstr);
		AbaxCStr& operator +=( int ch);
		AbaxCStr& append( const char *cpstr, unsigned long len);
		AbaxCStr& operator =( const AbaxCStr& cpstr) ;
		AbaxCStr& operator =( int c);
		AbaxCStr  operator+( const AbaxCStr& cpstr) const ;
		AbaxCStr& operator +( int ch);
        AbaxCStr& appendChars( int N, int ch );

		bool operator ==( const char *str) const;
		bool operator ==( const AbaxCStr &str) const;
		bool operator !=( const char *str) const;
		bool operator !=( const AbaxCStr &str) const;
		bool operator < ( const AbaxCStr &str) const;
		bool operator <= ( const AbaxCStr &str) const;
		bool operator > ( const AbaxCStr &str) const;
		bool operator >= ( const AbaxCStr &str) const;
		int operator[] (int i) const;
		int compare( const AbaxCStr &str) const;

		AbaxCStr& trimSpaces(int end=1); // strip ' ', \t, \n, \r at both ends
		AbaxCStr& trimEndChar( char chr ); // strip c at end
		AbaxCStr& trimChar(char c); // strip c at both ends
		void trimNull(); // strip NULL at end 

		//int  match(const AbaxCStr &regexp ) const;  // wheather the AbaxCStr matches a regular expression 
		//int  match(char *regexp ) const;  // wheather the AbaxCStr matches a regular expression 

		void replace( char oldc, char newc ); // replace every occurrence of old by new	
		void replace( const char * chrset, char newc ); // replace each char in chset by new	

		void replace( const char *oldstr, const char *newstr); // replace all oldstr in AbaxCStr by newstr
		void removeString( const char *oldstr ); // replace all oldstr in AbaxCStr 

		void remove( char c ); // remove every occurrence of c
		void remove( const char * chrset  ); // remove every char in chset 
		bool isNull() const;
		bool isNotNull() const;

		int			countChars(char c) const;
		AbaxCStr	substr(int start, int len ) const;
		AbaxCStr	substr(int start ) const;
		AbaxCStr&	pad0(); 
		void		destroy();
		void 		toUpper();
		void 		toLower();
		int     	caseEqual(const char *str) const;
		int     	numPunct() const;
		void    	print() const;
		AbaxCStr&   trimEndZeros();
		AbaxCStr& 	trim0() { return trimEndZeros(); }
		AbaxCStr 	firstToken( char sep );
		const char *secondTokenStart( char sep );
		AbaxCStr 	substrc( char startc, char endc ) const;
		bool        containsChar( char c ) const;
		bool        containsStr( const char *substr ) const;
		bool        containsStrCase( const char *substr, AbaxCStr& ret ) const;
		bool        isNumeric() const;  // 2 203 or 234.5
		bool        isDigit() const;  // 2 203 
		bool        isAllZero() const;  // 000
		void   	    dump();
		void        setDtype( const char *typ );
		bool        caseMatch( const char *str, const char *skips);
		bool        containAllWords( const char *words, const char *skips, bool ignoreCase );
		bool        containAnyWord( const char *words, const char *skips, bool ignoreCase );
		size_t	    hashCode() const;
        AbaxCStr    condenseSpaces();


		char    	dtype[4]; // data type of content

	private:
		bool    _readOnly;
		char 	*buf_;
		ssize_t 	length_;
		ssize_t 	capacity_;
		int  	    nseg_;
		static const   int ASTRSIZ=16;

		void allocMoreMemory( int len2 ); // len2 second string's length
		void initMem( int size ); 



};

typedef AbaxCStr Jstr;

// non-member
// AbaxCStr operator+ (const char *s1, const AbaxCStr &s2 );
// AbaxCStr operator+ (const AbaxCStr &s1, const AbaxCStr &s2 );
AbaxCStr operator+ (const char *s1, const AbaxCStr &s2 );
//AbaxCStr operator+ (const AbaxCStr &s1, const AbaxCStr &s2 );

#endif

