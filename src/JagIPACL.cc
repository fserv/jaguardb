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

#include <JagIPACL.h>
#include <JagUtil.h>
#include <JagStrSplit.h>

JagIPACL::JagIPACL( const Jstr  &fpath )
{
	_map = new JagHashMap<AbaxString, AbaxString>( true, 16 );

	// read from conf files
	readFile( fpath );
}

int JagIPACL::size() 
{
	return _map->size();
}

void JagIPACL::destroy()
{
	if ( _map ) {
		delete _map;
		_map = NULL;
		jagmalloc_trim( 0 );
	}
}

bool JagIPACL::readFile( const Jstr &fpath )
{
	// d("s7331 readFile fpath=[%s]\n", fpath.c_str() );
	FILE *fp = jagfopen( fpath.c_str(), "r" );
	if ( ! fp ) return false;

	char buf[256];
	Jstr line;
	_data = "";
	while ( NULL != fgets(buf, 256, fp ) ) {
		line = buf;
		line = trimTailLF( line );
		line = trimTailChar( line, ' ' );
		line = trimTailChar( line, '.' );
		// d("s2238 added line=[%s]\n", line.c_str() );
		_map->addKeyValue( line, "1" );
		_data += line + "\n";
	}

	jagfclose( fp );
	return true;
}

// ip:  192.178.2.123  in hashmap:  192.168
bool JagIPACL::match(  const Jstr& ip )
{
	// 192.178.2.123 in hashmap?
	// 192.178.2 in hashmap?
	// 192.178 in hashmap?
	// 192 in hashmap?
	// d("ss9393 check [%s]\n", ip.c_str() );
	if ( _map->keyExist( ip ) ) return true;

	AbaxString key;
	JagStrSplit sp( ip.c_str(), '.');
	if ( sp.length() < 4 ) return true;

	key = sp[0] + "." + sp[1] + "." + sp[2];
	// d("ss9391 check [%s]\n", key.c_str() );
	if ( _map->keyExist( key ) ) return true;


	key = sp[0] + "." + sp[1];
	// d("ss9591 check [%s]\n", key.c_str() );
	if ( _map->keyExist( key ) ) return true;

	key = sp[0];
	if ( _map->keyExist( key ) ) return true;

	return false;
}

void JagIPACL::refresh( const Jstr &newdata )
{
	_data = newdata;
	delete _map;
	_map = new JagHashMap<AbaxString, AbaxString>( true, 16 );
	JagStrSplit sp( _data, '\n');
	for ( int i = 0; i < sp.length(); ++i ) {
		_map->addKeyValue( sp[i], "1" );
	}
}


