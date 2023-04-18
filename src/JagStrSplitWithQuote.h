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
#ifndef _jag_strsplit_with_quote_h_
#define _jag_strsplit_with_quote_h_

#include <abax.h>

class  JagSplitPosition
{
   public:
	   const char *parsestart, *savestart, *saveend;
};

class JagStrSplitWithQuote
{
	public:   

		JagStrSplitWithQuote();
		//JagStrSplitWithQuote(const char* str, char sep = ';');
		JagStrSplitWithQuote(const Jstr &str, char sep, bool skipBracket=true, bool ignorereagion=false );
		JagStrSplitWithQuote(const char* str, char sep, bool skipBracket=true, bool ignorereagion=false );
		void init(const char* str, char sep, bool skipBracket=true, bool ignorereagion=false );
		int count(const char* str, char sep, bool skipBracket=true);

		void destroy();
		~JagStrSplitWithQuote();

	    const Jstr& operator[]( int i ) const;
		jagint length() const;
		jagint size() const;
		bool  exists(const Jstr &token) const;
		void	print();

	private:
		Jstr *list_;
		jagint length_;
		char sep_;
};

#endif

