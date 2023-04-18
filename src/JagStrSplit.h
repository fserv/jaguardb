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
#ifndef _jag_strsplit_h_
#define _jag_strsplit_h_

#include <abax.h>

class JagStrSplit
{
	public:   

		JagStrSplit();
		JagStrSplit(const Jstr& str, char sep = ' ', bool ignoreregion=false );
		JagStrSplit(const char *str, char sep = ' ', bool ignoreregion=false );

		JagStrSplit(const Jstr& str, int fields, char sep = ' ', bool ignoreregion=false );
		
		void init(const char *str, int fields, char sep=' ', bool ignoreregion=false );

		void destroy();
		~JagStrSplit();

	    const Jstr&     operator[](int i ) const;
	    Jstr&           operator[](int i );
		Jstr&           last();
		const Jstr&     last() const;
		jagint          length() const;
		jagint          size() const;
		jagint          slength() const;
		bool            exists(const Jstr &token) const;
		bool            contains(const Jstr &token, Jstr &rec ) const;
		void	        print() const;
		void	        printStr() const;
		void            shift();
		void            back();
		const char      *c_str() const;
		void            pointTo( const char* str );

	private:
		Jstr        *list_;
		jagint      length_;
		char        sep_;
		int         start_;
		Jstr        _NULL;
		const char* pdata_;
};

#endif

