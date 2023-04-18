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
#include <JagHashStrStr.h>
#include <JagUtil.h>

JagHashStrStr::JagHashStrStr()
{
	jag_hash_init( &_hash, 10 );
	_len = 0;
}

JagHashStrStr::~JagHashStrStr()
{
	jag_hash_destroy( &_hash );
}

bool JagHashStrStr::addKeyValue( const Jstr & key, const Jstr &val )
{
	if ( key.size() < 1 ) return 0;
	int rc = jag_hash_insert( &_hash, key.c_str(), val.c_str() );
	if ( rc ) {
		++ _len;
		return true;
	} 
	return false;
}

void JagHashStrStr::removeKey( const Jstr & key )
{
	if ( key.size() < 1 ) return;
	int rc = jag_hash_delete( &_hash, key.c_str() );
	if ( rc ) {
		-- _len;
	}
}

bool JagHashStrStr::keyExist( const Jstr & key ) const
{
	if ( key.size() < 1 ) return false;
	char *pval = jag_hash_lookup( &_hash, key.c_str() );
	if ( pval ) return true;
	return false;
}

char* JagHashStrStr::getValue( const Jstr &key ) const
{
	return jag_hash_lookup( &_hash, key.c_str() );
}

Jstr JagHashStrStr::getValue( const Jstr &key, bool &rc ) const
{
	char *p = jag_hash_lookup( &_hash, key.c_str() );
	if ( p ) {
		rc = true;
		return p;
	} else {
		rc = false;
		return "";
	}
}

bool JagHashStrStr::getValue( const Jstr &key, Jstr &val ) const
{
	char *p = jag_hash_lookup( &_hash, key.c_str() );
	if ( p ) {
		val = p;
		return true;
	} else {
		val = "";
		return false;
	}
}

void JagHashStrStr::reset()
{
	jag_hash_destroy( &_hash );
	jag_hash_init( &_hash, 10 );
	_len = 0;
}


JagHashStrStr::JagHashStrStr( const JagHashStrStr &o )
{
	jag_hash_init( &_hash, 10 );
	_len = 0;

	HashNodeT *node;
	for ( int i = 0; i < o._hash.size; ++i ) {
		node = o._hash.bucket[i];
		while ( node != NULL ) {
			jag_hash_insert( &_hash, node->key, node->value );
			node = node->next;
			++ _len;
		}
	}
}

JagHashStrStr& JagHashStrStr:: operator= ( const JagHashStrStr &o )
{
	if ( this == &o ) return *this;

	reset();

	HashNodeT *node;
	for ( int i = 0; i < o._hash.size; ++i ) {
		node = o._hash.bucket[i];
		while ( node != NULL ) {
			jag_hash_insert( &_hash, node->key, node->value );
			node = node->next;
			++ _len;
		}
	}

	return *this;
}

Jstr JagHashStrStr::getKVStrings( const char *sep)
{
	Jstr res;

	HashNodeT *node;
	for ( int i = 0; i < _hash.size; ++i ) {
		node = _hash.bucket[i];
		while ( node != NULL ) {
			if ( res.size() < 1 ) {
				res = Jstr(node->key) + "=" + node->value;
			} else {
				res += Jstr(sep) + Jstr(node->key) + "=" + node->value;
			}
			node = node->next;
		}
	}

	return res;
}

Jstr JagHashStrStr::getKeyStrings( const char *sep)
{
	Jstr res;

	HashNodeT *node;
	for ( int i = 0; i < _hash.size; ++i ) {
		node = _hash.bucket[i];
		while ( node != NULL ) {
			if ( res.size() < 1 ) {
				res = Jstr(node->key);
			} else {
				res += Jstr(sep) + Jstr(node->key);
			}
			node = node->next;
		}
	}

	return res;
}


int JagHashStrStr::size()
{
	return _len;
}

void JagHashStrStr::removeAllKey()
{
	reset();
}

void JagHashStrStr::clean()
{
	reset();
}

