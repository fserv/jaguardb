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
#ifndef _jag_hash_str_str_h_
#define _jag_hash_str_str_h_

#include <abax.h>
#include <JagVector.h>
#include <jaghashtable.h>
class JagHashStrStr
{
    public:
		JagHashStrStr();
		~JagHashStrStr();
		JagHashStrStr( const JagHashStrStr &o );
		JagHashStrStr& operator= ( const JagHashStrStr &o );

		bool addKeyValue( const Jstr &key, const Jstr &value );
		void removeKey( const Jstr &key );
		bool keyExist( const Jstr &key ) const;
		Jstr getValue( const Jstr &key, bool &rc ) const;
		char *getValue( const Jstr &key ) const;
		void reset();
		bool getValue( const Jstr &key, Jstr &val ) const;
		Jstr getKVStrings( const char *sep = "|" );
		Jstr getKeyStrings( const char *sep = "|" );
		int  size(); // how many items
		void removeAllKey();
		void clean();


	protected:
		jag_hash_t  _hash;
		int     _len;
};


#endif
