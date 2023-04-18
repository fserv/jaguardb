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
#include <JagHashStrPtr.h>

JagHashStrPtr::JagHashStrPtr( int storeType )
{
	jag_hash_init( &_hash, 10 );
	_len = 0;
	_type = storeType;
	if ( _type == JAG_INLINE_STORE ) {
		_deleteValue = true;
	} else {
		_deleteValue = false;
	}
}

JagHashStrPtr::~JagHashStrPtr()
{
	jag_hash_destroy( &_hash, _deleteValue );
}

bool JagHashStrPtr::addKeyValue( const Jstr & key, void *ptr )
{
	if ( key.size() < 1 ) return 0;
	int rc = jag_hash_insert_str_voidptr( &_hash, key.c_str(), ptr );
	if ( rc ) {
		++ _len;
		return true;
	} 
	return false;
}

void JagHashStrPtr::removeKey( const Jstr & key )
{
	if ( key.size() < 1 ) return;
	int rc = jag_hash_delete( &_hash, key.c_str(), _deleteValue );
	if ( rc ) {
		-- _len;
	}
}

bool JagHashStrPtr::keyExist( const Jstr & key ) const
{
	if ( key.size() < 1 ) return false;
	char *pval = jag_hash_lookup( &_hash, key.c_str() );
	if ( pval ) return true;
	return false;
}

void* JagHashStrPtr::getValue( const Jstr &key ) const
{
	return (void*)jag_hash_lookup( &_hash, key.c_str() );
}

void JagHashStrPtr::reset()
{
	jag_hash_destroy( &_hash, _deleteValue );
	jag_hash_init( &_hash, 10 );
	_len = 0;
}


JagHashStrPtr::JagHashStrPtr( const JagHashStrPtr &o )
{
	jag_hash_init( &_hash, 10 );

	_len = 0;
	HashNodeT *node;
	for ( int i = 0; i < o._hash.size; ++i ) {
		node = o._hash.bucket[i];
		while ( node != NULL ) {
			jag_hash_insert_str_voidptr( &_hash, node->key, node->value );
			node = node->next;
			++ _len;
		}
	}
}

JagHashStrPtr& JagHashStrPtr:: operator= ( const JagHashStrPtr &o )
{
	if ( this == &o ) return *this;

	reset();

	HashNodeT *node;
	for ( int i = 0; i < o._hash.size; ++i ) {
		node = o._hash.bucket[i];
		while ( node != NULL ) {
			jag_hash_insert_str_voidptr( &_hash, node->key, node->value );
			node = node->next;
			++ _len;
		}
	}

	return *this;
}

Jstr JagHashStrPtr::getKeyStrings( const char *sep)
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


int JagHashStrPtr::size()
{
	return _len;
}

void JagHashStrPtr::removeAll()
{
	reset();
}

int JagHashStrPtr::getValueArray( VoidPtr *arr, int arrlen )
{
	HashNodeT *node;
	int cnt = 0;
	for ( int i = 0; i < _hash.size; ++i ) {
		node = _hash.bucket[i];
		while ( node != NULL ) {
			arr[cnt] = (VoidPtr)node->value;
			node = node->next;
			++ cnt;
		}
	}

	return cnt;
}


