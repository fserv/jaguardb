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
#include <JagWalMap.h>

JagWalMap::JagWalMap()
{
	_map = new JagHashStrPtr( JAG_OUTLINE_STORE );
}

JagWalMap::~JagWalMap()
{
	delete _map;
}

int JagWalMap::size()
{
	return _map->size();
}

FILE *JagWalMap::ensureFile( const Jstr &fpath )
{
	void* ptr = _map->getValue( fpath );
	if ( ptr ) {
		return (FILE*)ptr;
	}

    FILE *f = fopen( fpath.c_str(), "ab" );
	if ( f == NULL ) {
		return NULL;
	}

    _map->addKeyValue( fpath, (void*)f );
    return f;
}

void JagWalMap::removeKey( const Jstr &fpath )
{
	 _map->removeKey( fpath );

}

void  JagWalMap::closeAllFiles()
{
	int arrlen = _map->size();
	VoidPtr *arr = new VoidPtr[arrlen];

	_map->getValueArray( arr, arrlen );
	for ( int i=0; i < arrlen; ++i ) {
		fclose( (FILE*)arr[i] );
	}
	delete [] arr;
}


