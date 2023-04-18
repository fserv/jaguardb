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
#ifndef _jag_hash_str_int_h_
#define _jag_hash_str_int_h_

#include <abax.h>
#include <JagVector.h>
//#include <jaghashtable.h>
#include <unordered_map>

struct JstrHasher
{
	std::size_t operator() (const Jstr &k) const
	{
		return k.hashCode();
	};
};

class JagHashStrInt
{
    public:
		/**
		JagHashStrInt();
		~JagHashStrInt();
		**/
		//JagHashStrInt( const JagHashStrInt &o );
		//JagHashStrInt& operator= ( const JagHashStrInt &o );

		bool addKeyValue( const Jstr &key, int val );
		void removeKey( const Jstr &key );
		bool keyExist( const Jstr &key ) const;
		int getValue( const Jstr &key, bool &rc ) const;
		void reset();
		bool getValue( const Jstr &key, int &pos ) const;
		int  size(); // how many items
		void removeAllKey();
		void print() const;
		JagVector<AbaxPair<Jstr,jagint> > getStrIntVector();


	protected:
		//jag_hash_t  _hash;
		std::unordered_map<Jstr, int, JstrHasher> _hash;
		//int     _len;
};


#endif
