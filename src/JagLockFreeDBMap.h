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
#ifndef _jag_lockfree_dbmap_
#define _jag_lockfree_dbmap_

#include <cds/container/skip_list_map_hp.h>
#include <cds/gc/hp.h>      // for cds::HP (Hazard Pointer) SMR
#include <cds/init.h>       // for cds::Initialize and cds::Terminate
#include <abax.h>

class JagLockFreeDBMap
{
    public:
		//typedef cds::container::SkipListMap< cds::gc::HP, JagFixString, JagFixString > JagMapType;
		//typedef cds::container::SkipListMap< cds::gc::HP, std::string, std::string > JagMapType;
		// typedef std::ios_base::fmtflags flags;
		using JagMapType = cds::container::SkipListMap< cds::gc::HP, JagFixString, JagFixString>;
		class JagFunctor {
		   public:
		     JagFixString value;
			 void operator()( bool bNew, JagMapType::value_type &item )
			 //void operator()( bool bNew, cds::container::SkipListMap< cds::gc::HP, std::string, std::string >::value_type &item )
			 {
			 	item.second = value;
			 }
		};

		JagLockFreeDBMap( int bufSize);
		~JagLockFreeDBMap();
		bool insert( const JagFixString& key, const JagFixString& value );
		bool exists( const JagFixString& key );
		bool get( const JagFixString& key, JagFixString& value );
		bool remove( const JagFixString &key );
		bool update( const JagFixString& key, const JagFixString& value );
		jagint  size() { return _length; } 

		/***
        class iterator
        {
			protected:
            	const Container &sln;
            	size_t m_Index;
        	public:
                iterator(const Container &s): sln(s),m_Index(0)
                {
         
                }
                void operator++()
                {
                    m_Index++;
                }
         
                void operator--()
                {
                    m_Index++;
                }
         
                bool operator != (const iterator& other)
                {
                    return m_Index != other.m_Index;
                }
         
                int operator *()
                {
                    return sln.m_Elements[m_Index];
                }
        };
         
        iterator begin()
        {
            iterator it(*this);
            return it;
        }
         
        iterator end()
        {
            iterator it(*this, m_Elements.size());
            return it;
        }
		***/

	protected:
		JagMapType      _map;
		pthread_mutex_t _deleteMutex;
		int             _bufferSize;
		std::pair<JagFixString,JagFixString>  *_buffer;
		jagint          _length;
};

JagLockFreeDBMap::JagLockFreeDBMap( int bufSize)
{
	_bufferSize = bufSize;
	_length = 0;
	// _buffer = new std::pair<JagFixString,JagFixString>[_bufferSize];
	_buffer = NULL;
	pthread_mutex_init( &_deleteMutex, NULL );
}

JagLockFreeDBMap::~JagLockFreeDBMap()
{
	pthread_mutex_destroy( &_deleteMutex );
	if ( _buffer ) {
		delete [] _buffer;
	}
}

bool JagLockFreeDBMap::insert( const JagFixString& key, const JagFixString& value )
{
	bool rc = _map.insert( key, value );
	if ( rc ) ++ _length;
	return rc;
}

bool JagLockFreeDBMap::exists( const JagFixString& key )
{
	return _map.contains( key );
}

bool JagLockFreeDBMap::get( const JagFixString& key, JagFixString &value )
{
	JagMapType::guarded_ptr ptr( _map.get( key ));
	if ( ptr ) {
		value = ptr->second;
		return true;
	}
	return false;
}

bool JagLockFreeDBMap::remove( const JagFixString &key )
{
	bool rc;
	pthread_mutex_lock ( &_deleteMutex );
	rc = _map.erase( key );
	if ( rc ) -- _length;
	pthread_mutex_unlock ( &_deleteMutex );
	return rc;
}

bool JagLockFreeDBMap::update( const JagFixString& key, const JagFixString& value )
{
	JagFunctor   functor;
	functor.value = value;
	std::pair<bool, bool> res = _map.update( key, functor, false );
	return res.first;
}


#endif
