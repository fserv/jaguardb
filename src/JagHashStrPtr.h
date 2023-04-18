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
#ifndef _jag_hash_str_ptr_h_
#define _jag_hash_str_ptr_h_

#include <abax.h>
#include <JagVector.h>
#include <jaghashtable.h>
#include <pthread.h>
class JagHashStrPtr
{
    public:
		JagHashStrPtr( int storeType );
		~JagHashStrPtr();
		JagHashStrPtr( const JagHashStrPtr &o );
		JagHashStrPtr& operator= ( const JagHashStrPtr &o );

		bool addKeyValue( const Jstr &key, void *ptr );
		void removeKey( const Jstr &key );
		bool keyExist( const Jstr &key ) const;
		void *getValue( const Jstr &key ) const;
		void reset();
		int  size(); // how many items
		void removeAll();
		Jstr getKeyStrings( const char *sep);
		pthread_mutex_t *ensureMutex( const Jstr &key );
		int getValueArray( VoidPtr *arr, int arrlen );

	protected:
		jag_hash_t  _hash;
		int     _len;
		int     _type;
		bool    _deleteValue;
};


#endif
