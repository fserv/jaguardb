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

#ifndef _jag_hash_lock_
#define _jag_hash_lock_

#include <pthread.h>
#include <abax.h>
#include <JagHashMap.h>

class JagHashLock
{
  public:
  	JagHashLock();
	~JagHashLock();
	void readLock( const AbaxString &kstr );
	void readUnlock( const AbaxString &kstr );

	void writeLock( const AbaxString &kstr );
	void writeUnlock( const AbaxString &kstr );

	jagint getWriters() const { return _writers; }
	jagint getReaders() const { return _readers; }

  protected:
  	pthread_mutex_t  _mutex;
	pthread_cond_t   _condvar;
	JagHashMap<AbaxString, AbaxLong2> *_map;
	jagint			 _readers;
	jagint			 _writers;

	void init();
	bool regionOverlaps( const AbaxString &kstr, bool isRead );

};


#endif
