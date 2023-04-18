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

#include <JagMutex.h>
#include <JagUtil.h>


JagMutex::JagMutex( pthread_mutex_t *mutex )
{
	_mutex = mutex;
	pthread_mutex_lock( _mutex );
}

JagMutex::~JagMutex( )
{
	pthread_mutex_unlock( _mutex );
}

void JagMutex::lock()
{
	pthread_mutex_lock( _mutex );
}

void JagMutex::unlock()
{
	pthread_mutex_unlock( _mutex );
}



JagReadWriteMutex::JagReadWriteMutex( pthread_rwlock_t *lock )
{
	_lock = lock;
	_type = 0;
}

JagReadWriteMutex::JagReadWriteMutex( pthread_rwlock_t *lock, int type )
{
	_lock = lock;
	_type = type;

	if ( type != READ_LOCK && type != WRITE_LOCK ) {
		_type = WRITE_LOCK;
	}

	if ( _type == READ_LOCK ) {
    	JAG_BLURT 
		if ( _lock )  pthread_rwlock_rdlock( _lock );
    	JAG_OVER
	} else {
    	JAG_BLURT 
		if ( _lock ) pthread_rwlock_wrlock( _lock );
    	JAG_OVER
	}
}

JagReadWriteMutex::~JagReadWriteMutex( )
{
	if ( _type == READ_LOCK ) {
		if ( _lock ) pthread_rwlock_unlock( _lock );
	} else if (  _type == WRITE_LOCK ) {
		if ( _lock ) pthread_rwlock_unlock( _lock );
	} 
}

void JagReadWriteMutex::readLock()
{
	_type = READ_LOCK;
	JAG_BLURT
	if ( _lock ) pthread_rwlock_rdlock( _lock );
	JAG_OVER
}

void JagReadWriteMutex::readUnlock()
{
	_type = 0;
	if ( _lock ) pthread_rwlock_unlock( _lock );
}


void JagReadWriteMutex::writeLock()
{
	_type = WRITE_LOCK;
	JAG_BLURT
	if ( _lock ) pthread_rwlock_wrlock( _lock );
	JAG_OVER
}

void JagReadWriteMutex::writeUnlock()
{
	_type = 0; 
	if ( _lock ) pthread_rwlock_unlock( _lock );
}

void JagReadWriteMutex::unlock()
{
	if ( ! _lock ) return;

	pthread_rwlock_unlock( _lock );
	_type = 0; 
}

/**
void JagReadWriteMutex::upgradeToWriteLock()
{
	_type = WRITE_LOCK ;
	JAG_BLURT
	if ( _lock ) _lock->upgradeToWriteLock();
	JAG_OVER
}
**/

pthread_rwlock_t *newJagReadWriteLock()
{
	pthread_rwlock_t *rwlock = new pthread_rwlock_t();
	pthread_rwlock_init(rwlock, NULL);
	return rwlock;
}

void deleteJagReadWriteLock( pthread_rwlock_t *lock )
{
	if ( NULL == lock ) return;
	pthread_rwlock_destroy(lock);
	delete lock;
}


