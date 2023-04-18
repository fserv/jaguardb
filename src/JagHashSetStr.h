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
#ifndef _jag_hash_str_set_h_
#define _jag_hash_str_set_h_

#include <abax.h>
#include <JagVector.h>
#include <jaghashset.h>
class JagHashSetStr
{
    public:
		JagHashSetStr();
		~JagHashSetStr();
		JagHashSetStr( const JagHashSetStr &o );
		JagHashSetStr& operator= ( const JagHashSetStr &o );

		bool addKey( const Jstr &key );
		void removeKey( const Jstr &key );
		bool keyExist( const Jstr &key ) const;
		void reset();
		int  size(); // how many items
		void removeAllKey();
		void print();

	protected:
		jag_hash_set_t  _hash;
		int     _len;
};


#endif
