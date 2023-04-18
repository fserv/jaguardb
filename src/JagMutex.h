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
#ifndef _jag_mutex_h_
#define _jag_mutex_h_

#include <stdio.h>
#include <pthread.h>

// simple mutex wrapper
class JagMutex
{
  public:
    JagMutex( pthread_mutex_t *_mutex );
	void lock();
	void unlock();
    ~JagMutex( );

  protected:
	pthread_mutex_t  *_mutex;
};

/////////// Read and write mutex for readers and writers
class JagReadWriteMutex
{
  public:
    JagReadWriteMutex( pthread_rwlock_t *lock );
    JagReadWriteMutex( pthread_rwlock_t *lock, int lockType );
    ~JagReadWriteMutex( );
	void readLock();
	void writeLock();
	void readUnlock();
	void writeUnlock();
	void unlock();
	//void upgradeToWriteLock();

	static  const  int  READ_LOCK = 1;
	static  const  int  WRITE_LOCK = 2;

  protected:
	pthread_rwlock_t  *_lock;
	int				   _type;  
};

pthread_rwlock_t *newJagReadWriteLock();
void deleteJagReadWriteLock( pthread_rwlock_t *lock );

#endif
