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

#ifndef _jag_block_lock_
#define _jag_block_lock_

#include <pthread.h>
#include <abax.h>
#include <JagHashMap.h>

class JagBlockLock
{
  public:
  	JagBlockLock();
	~JagBlockLock();
	void readLock( jagint pos=-1 );
	void readUnlock( jagint pos=-1 );

	void writeLock( jagint pos=-1 );
	void writeUnlock( jagint pos=-1 );

  protected:
  	pthread_mutex_t  _mutex;
	pthread_cond_t   _condvar;
	JagHashMap<AbaxLong, AbaxLong2> *_map;
	jagint			 _readers;
	jagint			 _writers;

	void init();
	bool regionOverlaps( jagint pos, bool isRead );

};


#endif
