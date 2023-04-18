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
#ifndef _jag_hash_int_int_h_
#define _jag_hash_int_int_h_

#include <pthread.h>
#include <abax.h>
#include <JagVector.h>
#include <jaghashtable.h>
class JagHashIntInt
{
    public:
		JagHashIntInt( bool useLock = true);
		~JagHashIntInt();
		JagHashIntInt( const JagHashIntInt &o );
		JagHashIntInt& operator= ( const JagHashIntInt &o );

		bool addKeyValue( int, int val );
		void removeKey( int key );
		bool keyExist( int key ) const;
		int  getValue( int key, bool &rc ) const;
		bool getValue( int key, int &pos ) const;
		void reset();
		int  size(); // how many items
		void removeAllKey();
		void print();
		JagVector<AbaxPair<int,int> > getIntIntVector();
		JagVector<int> getIntVector();


	protected:
		jag_hash_t  _hash;
		int     	_len;
		bool        _useLock;
		pthread_rwlock_t  _lock;
};


#endif
