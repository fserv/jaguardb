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
#include <JagHashLock.h>

JagHashLock::JagHashLock()
{
	init();
}

JagHashLock::~JagHashLock()
{
	if ( _map ) delete _map;
	_map = NULL;
}

void JagHashLock::init()
{
	_map = new JagHashMap<AbaxString, AbaxLong2>(100);
  	pthread_mutex_init(& _mutex, (pthread_mutexattr_t *)0);
  	pthread_cond_init(&_condvar, (pthread_condattr_t *)0);
	_readers = _writers = 0;
}

bool JagHashLock::regionOverlaps( const AbaxString &kstr, bool isRead )
{

    if ( kstr == "-1" ) {
        if ( ! isRead ) {
            if ( _readers > 0 ||  _writers > 0 ) {
                return true;
            } else {
                return false;  
            }
        } else {
            if ( _writers > 0 ) {
                return true;  
            } else {
                return false;
            }
        }
    }


	if ( ! isRead ) {
		if ( _map->keyExist( "-1" ) ) {
			return true;
		}

    	if ( _map->keyExist( kstr ) ) {
    		return true; 
    	}
    
    	return false;
	} else {
		AbaxLong2 cn;
		if ( _map->getValue( "-1", cn ) ) {
			if ( cn.data2 < 1 ) {
				return false;
			} else {
				return true;
			}
		}

		if ( _map->getValue( kstr, cn ) ) {
			if ( cn.data2 < 1 ) {
				return false;  
			} else {
				return true;
			}
		}

    	return false;
	}

	return false;
}

void JagHashLock::writeLock( const AbaxString & kstr )
{
	JAG_BLURT
    pthread_mutex_lock( & _mutex);
    while ( regionOverlaps( kstr, false ) ) {
      pthread_cond_wait(&_condvar, & _mutex);
    }
    AbaxLong2 v2;
    _map->getValue( kstr, v2 );
    ++ v2.data2;
	++ _writers;
    _map->setValue( kstr, v2, true );
    pthread_mutex_unlock(&_mutex);
	JAG_OVER
}
  
void JagHashLock::writeUnlock( const AbaxString & kstr )
{
	JAG_BLURT
    pthread_mutex_lock(&_mutex);
    AbaxLong2 v2;
    _map->getValue( kstr, v2 );
    if ( v2.data2 == 1 && v2.data1 == 0 ) {
		-- _writers;
    	_map->removeKey( kstr );
    } else {
		if ( v2.data2 > 0 ) {
    		-- v2.data2;
			-- _writers;
    		_map->setValue( kstr, v2, true );
		}
    }
    pthread_cond_broadcast(&_condvar);
    pthread_mutex_unlock(&_mutex);
	JAG_OVER
}
  
void JagHashLock::readLock( const AbaxString & kstr )
{
	JAG_BLURT
    pthread_mutex_lock( & _mutex);
    while ( regionOverlaps( kstr, true ) ) {
      pthread_cond_wait(&_condvar, & _mutex);
    }
    AbaxLong2 v2;
    _map->getValue( kstr, v2 );
    ++ v2.data1;
	++ _readers;
    _map->setValue( kstr, v2, true );
    pthread_mutex_unlock(&_mutex);
	JAG_OVER
}

void JagHashLock::readUnlock( const AbaxString & kstr )
{
	JAG_BLURT
    pthread_mutex_lock(&_mutex);
    AbaxLong2 v2;
    _map->getValue( kstr, v2 );
    if ( v2.data1 == 1 && v2.data2 == 0 ) {
		-- _readers;
    	_map->removeKey( kstr );
    } else {
		if ( v2.data1 > 0 ) {
    		-- v2.data1;
			-- _readers;
    		_map->setValue( kstr, v2, true );
		}
    }
    pthread_cond_broadcast(&_condvar);
    pthread_mutex_unlock(&_mutex);
	JAG_OVER
}
