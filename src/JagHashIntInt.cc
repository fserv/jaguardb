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
#include <JagHashIntInt.h>
#include <JagUtil.h>

JagHashIntInt::JagHashIntInt( bool useLock )
{
	jag_hash_init( &_hash, 10 );
	_len = 0;

	_useLock = useLock;
	if ( _useLock ) {
		 pthread_rwlock_init((pthread_rwlock_t*)&_lock, NULL);
	}
}

JagHashIntInt::~JagHashIntInt()
{
	jag_hash_destroy( &_hash );
	if ( _useLock ) {
		 pthread_rwlock_destroy((pthread_rwlock_t*)&_lock);
	}
}

bool JagHashIntInt::addKeyValue( int key, int val )
{
	if ( _useLock ) { pthread_rwlock_wrlock((pthread_rwlock_t*) &_lock ); }
	int rc = jag_hash_insert_int_int( &_hash, key, val );
	if ( rc ) {
		++ _len;
		if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
		return true;
	} 
	if ( _useLock ) { pthread_rwlock_unlock((pthread_rwlock_t*)&_lock ); }
	return false;
}

void JagHashIntInt::removeKey( int key )
{
	if ( _useLock ) { pthread_rwlock_wrlock( (pthread_rwlock_t*)&_lock ); }
	int rc = jag_hash_delete_int( &_hash, key );
	if ( rc ) {
		-- _len;
	}
	if ( _useLock ) { pthread_rwlock_unlock( (pthread_rwlock_t*)&_lock ); }
}

bool JagHashIntInt::keyExist( int key ) const
{
	int val = 0;
	if ( _useLock ) { pthread_rwlock_rdlock( (pthread_rwlock_t*)&_lock ); }
	bool rc = jag_hash_lookup_int_int( &_hash, key, &val );
	if ( _useLock ) { pthread_rwlock_unlock( (pthread_rwlock_t*)&_lock ); }
	return rc;
}

int JagHashIntInt::getValue( int key, bool &rc ) const
{
	int val = 0;
	if ( _useLock ) { pthread_rwlock_rdlock( (pthread_rwlock_t*)&_lock ); }
	rc = jag_hash_lookup_int_int( &_hash, key, &val );
	if ( _useLock ) { pthread_rwlock_unlock( (pthread_rwlock_t*)&_lock ); }
	return val;
}

bool JagHashIntInt::getValue( int key, int &val ) const
{
	if ( _useLock ) { pthread_rwlock_rdlock( (pthread_rwlock_t*)&_lock ); }
	bool rc = jag_hash_lookup_int_int( &_hash, key, &val );
	if ( _useLock ) { pthread_rwlock_unlock( (pthread_rwlock_t*)&_lock ); }
	return rc;
}

void JagHashIntInt::reset()
{
	jag_hash_destroy( &_hash );
	jag_hash_init( &_hash, 10 );
	_len = 0;
}


JagHashIntInt::JagHashIntInt( const JagHashIntInt &o )
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

JagHashIntInt& JagHashIntInt:: operator= ( const JagHashIntInt &o )
{
	if ( _useLock ) { pthread_rwlock_wrlock( &_lock ); }
	if ( this == &o ) {
		if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
		return *this;
	}

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

	if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
	return *this;
}

void JagHashIntInt::print()
{
	i("s2378 JagHashIntInt::print():\n" );
	HashNodeT *node;
	for ( int j = 0; j < _hash.size; ++j ) {
		node = _hash.bucket[j];
		while ( node != NULL ) {
			i("124 key=[%s] ==> value=[%s]\n", node->key, node->value );
			node = node->next;
		}
	}
}


JagVector<AbaxPair<int,int>> JagHashIntInt::getIntIntVector()
{
	JagVector<AbaxPair<int,int>> vec;

	HashNodeT *node;
	if ( _useLock ) { pthread_rwlock_rdlock( &_lock ); }
	for ( int i = 0; i < _hash.size; ++i ) {
		node = _hash.bucket[i];
		while ( node != NULL ) {
			AbaxPair<int,int> pair(atoi(node->key), atoi(node->value) );
			vec.append( pair );
			node = node->next;
		}
	}

	if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
	return vec;
}

JagVector<int> JagHashIntInt::getIntVector()
{
	JagVector<int> vec;

	HashNodeT *node;
	if ( _useLock ) { pthread_rwlock_rdlock( &_lock ); }
	for ( int i = 0; i < _hash.size; ++i ) {
		node = _hash.bucket[i];
		while ( node != NULL ) {
			vec.append( atoi(node->key) );
			node = node->next;
		}
	}

	if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
	return vec;
}

int JagHashIntInt::size()
{
	if ( _useLock ) { pthread_rwlock_rdlock( &_lock ); }
	int len = _len;
	if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
	return len;
}

void JagHashIntInt::removeAllKey()
{
	if ( _useLock ) { pthread_rwlock_wrlock( &_lock ); }
	reset();
	if ( _useLock ) { pthread_rwlock_unlock( &_lock ); }
}

