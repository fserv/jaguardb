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
#include <JagDef.h>
#include <JagMutexMap.h>

// ctor
JagMutexMap::JagMutexMap()
{
	_map = new JagHashStrPtr( JAG_INLINE_STORE );
}

// dtor
JagMutexMap::~JagMutexMap()
{
	delete _map;
}

int JagMutexMap::size()
{
	return _map->size();
}

pthread_mutex_t *JagMutexMap::ensureMutex( const Jstr &key )
{
	void* ptr = _map->getValue( key );
	if ( ptr ) {
		return (pthread_mutex_t*)ptr;
	}

    pthread_mutex_t *mtx = new pthread_mutex_t;
    pthread_mutex_init( mtx, NULL );
    _map->addKeyValue( key, (void*)mtx );
    return mtx;
}

