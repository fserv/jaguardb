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
#ifndef _jag_hashmap_h_
#define _jag_hashmap_h_
#include <stdio.h>
#include <abax.h>

#include <JagMutex.h>
#include <JagHashArray.h>


//////////////////////////////////// HashMap ///////////////////////////////////
template <class K, class V>
class JagHashMap 
{
    template <class K2, class V2> friend class JagHashMapIterator;

    public: 

        JagHashMap( bool doLock = false,  int length=16 );
        ~JagHashMap();
		JagHashMap& operator=( const JagHashMap &map );

        bool addKeyValue( const K& key, const V& value );
        bool keyExist(  const K& key );
        bool getValue( const K& key, V &value ) const;
        V &getValue( const K& key, bool &rc );
        V* getValue( const K& key );
        V* getValuePtr( const K& key );
        bool setValue( const K& key, const V& value, bool force=false );
		bool appendValue( const K& key, const V &value );
        bool removeKey( const K& key, AbaxDestroyAction action=ABAX_NOOP  );
		void removeAllKey( int length=16 );
        jagint removeMatchKey( const K& key, AbaxDestroyAction action=ABAX_NOOP  );

        jagint size( ) const  { return _xarr->elements(); }
		const K& keyAt( jagint i ) const { return (*_xarr)[i].key; }
		const V& valueAt( jagint i ) const { return  (*_xarr)[i].value; }
		const AbaxPair<K,V> &pairAt( jagint i) const { return (*_xarr)[i]; }

		const AbaxPair<K,V> *array() const { return _xarr->array(); }
		const JagHashArray<AbaxPair<K,V> > *hashArray() const { return _xarr; }
        jagint arrayLength( ) const  { return _xarr->size(); }
		inline bool isNull( jagint i ) const { return _xarr->isNull(i); }

		void printKeyStringOnly() { _xarr->printKeyStringOnly(); }
		void printKeyIntegerOnly() { _xarr->printKeyIntegerOnly(); }
		void printKeyStringValueString() { _xarr->printKeyStringValueString(); }
		void printKeyIntegerValueString() { _xarr->printKeyIntegerValueString(); }
		void printKeyStringValueInteger() { _xarr->printKeyStringValueInteger(); }
		void printKeyIntegerValueInteger() { _xarr->printKeyIntegerValueInteger(); }
		pthread_rwlock_t    *_lock;

    protected:
        JagHashArray<AbaxPair<K,V> >  *_xarr; 
		void  destroy();
		bool  _doLock;
};

template <class K, class V>
JagHashMap<K,V>::JagHashMap( bool doLock, int length )
{
	_doLock = doLock;
	if ( _doLock ) {
		_lock = newJagReadWriteLock();
	} else {
		_lock = NULL;
	}
	// d("s2042 JagHashMap() ctor  this=%x thread=%lld _doLock=%d _lock=%0x\n", this, THREADID, _doLock, _lock );
	_xarr = new JagHashArray< AbaxPair<K,V> > ( length );
}

template <class K, class V>
JagHashMap<K,V>::~JagHashMap()
{
	// d("s2042 JagHashMap() dtor  this=%x thread=%lld _doLock=%d _lock=%0x\n", this, THREADID, _doLock, _lock );
	this->destroy();
}

template <class K, class V>
bool JagHashMap<K,V>::addKeyValue( const K& key, const V& value )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );

	AbaxPair<K,V> pair(key, value);
	return _xarr->insert( pair );
}

template <class K, class V>
bool JagHashMap<K,V>::keyExist( const K& key )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	AbaxPair<K,V> pair(key);
	return _xarr->exist( pair );
}

template <class K, class V>
bool JagHashMap<K,V>::getValue( const K& key, V &value ) const
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	bool rc;
	AbaxPair<K,V> pair(key);
	rc = _xarr->get( pair );
	if ( ! rc ) return false;

	value = pair.value;

	return true;
}

template <class K, class V>
V& JagHashMap<K,V>::getValue( const K& key, bool &rc )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	if ( NULL == _xarr ) {
		rc = false;
		static V v;
		return v; 
	}
	AbaxPair<K,V> inpair(key);
	AbaxPair<K,V> &pair = _xarr->get( inpair, rc );
	return pair.value;
}

template <class K, class V>
V* JagHashMap<K,V>::getValue( const K& key )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	AbaxPair<K,V> inpair(key);
	bool rc;
	AbaxPair<K,V> &pair = _xarr->get( inpair, rc );
	if ( ! rc ) return NULL;
	return &(pair.value);
}

template <class K, class V>
V* JagHashMap<K,V>::getValuePtr( const K& key )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	AbaxPair<K,V> inpair(key);
	bool rc;
	AbaxPair<K,V> &pair = _xarr->get( inpair, rc );
	if ( ! rc ) return NULL;
	return &(pair.value);
}

template <class K, class V>
bool JagHashMap<K,V>::setValue( const K& key, const V &value, bool force )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	AbaxPair<K,V> pair(key, value);
	bool rc = _xarr->set( pair );
	if ( force ) {
		if ( ! rc ) {
			rc = _xarr->insert( pair );
		}
	}
	return rc;
}

template <class K, class V>
bool JagHashMap<K,V>::appendValue( const K& key, const V &value )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	AbaxPair<K,V> pair(key, value);
	bool rc = _xarr->insert( pair );
	if ( !rc ) {
		AbaxPair<K,V> pair2(key);
		rc = _xarr->get( pair2 );
		if ( ! rc ) return false;
		pair.value = pair2.value + pair.value;
		rc = _xarr->set( pair );
	}
	return rc;
}

template <class K, class V>
bool JagHashMap<K,V>::removeKey( const K& key, AbaxDestroyAction action )
{
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	AbaxPair<K,V> pair(key);
	return _xarr->remove( pair, action );
}

template <class K, class V>
void JagHashMap<K,V>::removeAllKey( int length )
{
	//d("s3003 this=%x thread=%lld JagHashMap<K,V>::removeAllKey\n", this, THREADID );
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	if ( _xarr ) {
		delete _xarr; 
	}
	_xarr = new JagHashArray< AbaxPair<K,V> > ( length );
	// d("s3340 this=%lld thread=%lld JagHashMap new JagReadWriteLock _lock=%0x\n", this, THREADID, _lock );
}

template <class K, class V>
jagint JagHashMap<K,V>::removeMatchKey( const K& key, AbaxDestroyAction action )
{
	jagint cnt = 0;
	JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
	for ( jagint i = 0; i < _xarr->size(); ++i ) {
		if ( _xarr->isNull( i ) ) continue;
		const AbaxPair<K,V> &pair = (*_xarr)[i];
		if ( 0 == strncasecmp( pair.key.c_str(), key.c_str(), key.size() ) ) {
			_xarr->remove( pair, action );
			++cnt;
		}
	}
	return cnt;
}

template <class K, class V>
void JagHashMap<K,V>::destroy( )
{
	// d("s338383939 JagHashMap destroy() this=%0x  _doLock=%d  _lock=%0x\n", this, _doLock, _lock );
	{
		JagReadWriteMutex mutex( _lock, JagReadWriteMutex::WRITE_LOCK );
		if ( _xarr ) {
			delete _xarr; 
			_xarr = NULL;
		}
	}
	
	if ( _doLock ) {
		deleteJagReadWriteLock( _lock );
		_lock = NULL;
	}
}

template <class K, class V>
JagHashMap<K,V>& JagHashMap<K,V>:: operator=( const JagHashMap<K,V> &map )
{
	d("s22233038 JagHashMap  operator=   this=%0x _doLock=%d _lock=%0x\n", this, _doLock, _lock );
	/**
	if ( _doLock ) {
		deleteJagReadWriteLock( _lock );
		_lock = newJagReadWriteLock();
	} else {
		_lock = NULL;
	}
	**/

	if ( _xarr ) {
		delete _xarr;
	}

	_xarr = new JagHashArray< AbaxPair<K,V> >( *map._xarr );
}


//////////////////// JagHashMap Iterator //////////////////////////

/***
Iterator over an JagHashMap object
***/
template <class K, class V>
class JagHashMapIterator
{
    public: 

        JagHashMapIterator( JagHashMap<K,V> *hmap );
        JagHashMapIterator( JagHashMap<K,V> *hmap, jaguint count );

        ~JagHashMapIterator();

        void  begin();
        // inline JagHashMapIterator&  operator++() { nextStep(); return *this; }
        bool   hasNext();
        // AbaxKeyValue<K,V>  next( AbaxStepAction action=ABAX_NEXT );
        AbaxKeyValue<K,V>  next( );

    private:
        // void  nextStep(); 
        JagHashMap<K,V> *_mapobj; 


		jaguint 	_count;
		jaguint 	_itermax;
		unsigned char   _type; // 0: all;  10: has start;  20: has start and end element
		bool   			_doneBegin;

		jagint	    _cursor;
};

template <class K, class V>
JagHashMapIterator<K,V>::JagHashMapIterator( JagHashMap<K,V> *hmap )
{
	_mapobj = hmap;
	_count = 0;
	_type = ABAXITERALL;
	_doneBegin = false;
	begin();
}

template <class K, class V>
JagHashMapIterator<K,V>::JagHashMapIterator( JagHashMap<K,V> *hmap, jaguint count )
{
	_mapobj = hmap;
	_count = 0;
	this->_itermax = count;
	_type = ABAXITERCOUNT;
	_doneBegin = false;
	begin();
}

template <class K, class V>
JagHashMapIterator<K,V>::~JagHashMapIterator() 
{
	_mapobj = NULL;
	_count = 0 ;
}

template <class K, class V>
void JagHashMapIterator<K,V>::begin() 
{
	if ( this->_doneBegin ) {
		return;
	}

	if ( ! _mapobj ) return;

	this->_doneBegin = true;
	_cursor = _mapobj->_xarr->nextNonNull( _cursor );
}


template <class K, class V>
bool JagHashMapIterator<K,V>::hasNext() 
{
	// printf("c4409 hashmapiter hasNext: size=%d  cursor=%d\n", _mapobj->_xarr->size(), _cursor );

	if ( _cursor >= _mapobj->_xarr->size() ) {
		return false;
	}

	if ( ABAXITERCOUNT == this->_type ) {
		if ( this->_count >= this->_itermax )
		{
			_cursor = _mapobj->_xarr->size() + 1;  // mark end
			return false;
		}
	}

	return true;
}


// get ready for next ready data to be read
template <class K, class V>
AbaxKeyValue<K,V> JagHashMapIterator<K,V>::next() 
{
	this->_doneBegin = false;

	// printf("c4419 hashmapiter next: size=%d  cursor=%d\n", _mapobj->_xarr->size(), _cursor );

	if ( _cursor >= _mapobj->_xarr->size() ) {
		AbaxKeyValue<K,V> ref ( K::NULLVALUE, V::NULLVALUE );
		return ref;
	}

	AbaxPair<K,V> &pair = _mapobj->_xarr->at( _cursor );
	AbaxKeyValue<K,V> ref ( pair.key, pair.value );

	this->_count ++;
	++_cursor;
	_cursor = _mapobj->_xarr->nextNonNull( _cursor );

	return ref;
}

//////////////////////////////////// HashSet ///////////////////////////////////

template <class K>
class JagHashSet 
{

    template <class K2 > friend class JagHashSetIterator;
    template <class K2 > friend class JagHashSetReverseIterator;

    public: 

        JagHashSet(  );
        ~JagHashSet() { destroy(); }

        bool addKey( const K& key );
        bool keyExist(  const K& key );
        bool removeKey( const K& key );
        jagint size( ) const  { return _xarr->elements(); }
		// void setFormat( const char *fmt );
		void  destroy() { if ( _xarr ) delete _xarr; _xarr = NULL; }
		const K  *array() const { return _xarr->array(); }
        jagint arrayLength( ) const  { return _xarr->size(); }
		inline bool isNull( jagint i ) const { return _xarr->isNull(i); }

		void print() { _xarr->print(); }

    private:
        JagHashArray<K>  *_xarr; 

};

template <class K >
JagHashSet<K>::JagHashSet()
{
	_xarr = new JagHashArray<K> ();
}

template <class K >
bool JagHashSet<K>::addKey( const K& key )
{
	return _xarr->insert( key );
}

template <class K >
bool JagHashSet<K>::keyExist( const K& key )
{
	return _xarr->exist( key );
}

template <class K >
bool JagHashSet<K>::removeKey( const K& key )
{
	return _xarr->remove( key );
}


//////////////////// JagHashSet Iterator //////////////////////////

/***
Iterator over an JagHashSet object
***/
template <class K >
class JagHashSetIterator
{
    public: 

        JagHashSetIterator( JagHashSet<K> *hmap );
        JagHashSetIterator( JagHashSet<K> *hmap, jaguint count );

        ~JagHashSetIterator();

        void  begin();
        bool   hasNext();
        const K& next( );

    private:
        JagHashSet<K> *_mapobj; 

		jaguint 	_count;
		jaguint 	_itermax;
		unsigned char   _type; // 0: all;  10: has start;  20: has start and end element
		bool   			_doneBegin;

		jagint	    _cursor;
};

template <class K >
JagHashSetIterator<K>::JagHashSetIterator( JagHashSet<K> *hmap )
{
	_mapobj = hmap;
	_count = 0;
	_type = ABAXITERALL;
	_doneBegin = false;
	begin();
}

template <class K>
JagHashSetIterator<K>::JagHashSetIterator( JagHashSet<K> *hmap, jaguint count )
{
	_mapobj = hmap;
	_count = 0;
	this->_itermax = count;
	_type = ABAXITERCOUNT;
	_doneBegin = false;
	begin();
}


template <class K >
JagHashSetIterator<K>::~JagHashSetIterator() 
{
	_mapobj = NULL;
	_count = 0 ;
}

template <class K >
void JagHashSetIterator<K>::begin() 
{
	if ( this->_doneBegin ) {
		return;
	}

	if ( ! _mapobj ) return;

	this->_doneBegin = true;
	_cursor = _mapobj->_xarr->nextNonNull( _cursor );
}

template <class K>
bool JagHashSetIterator<K>::hasNext() 
{
	if ( _cursor >= _mapobj->_xarr->size() ) {
		return false;
	}

	if ( ABAXITERCOUNT == this->_type )
	{
		if ( this->_count >= this->_itermax )
		{
			_cursor = _mapobj->_xarr->size() + 1;  // mark end
			return false;
		}
	}

	return true;
}


// get ready for next ready data to be read
template <class K >
const K&  JagHashSetIterator<K>::next() 
{
	this->_doneBegin = false;

	if ( _cursor >= _mapobj->_xarr->size() ) {
		return K::NULLVALUE;
	}

	const K& key = _mapobj->_xarr->at( _cursor );

	this->_count ++;
	++_cursor;
	_cursor = _mapobj->_xarr->nextNonNull( _cursor );

	return key;
}


#endif
