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
#ifndef _jag_fix_string_h_
#define _jag_fix_string_h_

typedef long long jagint;

class JagFixString 
{
    public:
		static JagFixString NULLVALUE; 
        JagFixString();
        JagFixString( const char *str, unsigned int len ); 
        JagFixString( const char *str, unsigned int strlen, unsigned int capacity ); 
        JagFixString( const char *str ); 
        JagFixString( const JagFixString &str ); 
		JagFixString( const Jstr &str );
        JagFixString& operator=( const char *str );
        JagFixString& operator=( const JagFixString &str );
		JagFixString& operator=( const Jstr &str ); 
        int operator== ( const JagFixString &s2 )  const;
        int operator< ( const JagFixString &s2 ) const;
        int operator<= ( const JagFixString &s2 ) const;
        int operator> ( const JagFixString &s2 ) const;
        int operator>= ( const JagFixString &s2 ) const;
		JagFixString& operator+= (const JagFixString &s );
		JagFixString operator+ (const JagFixString &s ) const;
		void point(const char *str, unsigned int len );
		void point(const JagFixString &str );
		void strcpy( const char *data );
		~JagFixString();
		const char *addr() const { return _buf; }
		const char *c_str() const { return _buf; }
		const char *s() const { return _buf; }
		jagint addrlen() const { return _length; }
		jagint size() const { return _length; }
		jagint length() const { return _length; }
        void destroy() { };

        jagint hashCode() const ;
		void ltrim();
		void rtrim();
		void trim();
		void substr( jagint start, jagint length = -1 );
		JagFixString concat( const JagFixString& secondStr );
		void  print() const;
		void replace( char oldc, char newc );
		Jstr firstToken(char sep) const;
		char       dtype[4];
		void      setDtype( const char *typ );

	private:
		char       *_buf;
		jagint     _length;
		bool       _readOnly;
};

#endif
