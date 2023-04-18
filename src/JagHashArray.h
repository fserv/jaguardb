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
#ifndef _jag_hasharjag_h_
#define _jag_hasharjag_h_
#include <stdio.h>
#include <abax.h>

#include <JagMutex.h>
#include <JagUtil.h>


////////////////////////////////////////// array class ///////////////////////////////////

template <class Pair>
class JagHashArray
{
	public:

		JagHashArray( int size=256 );
		~JagHashArray();
		JagHashArray( const JagHashArray &arr );

		bool insert( const Pair &newpair ) { jagint idx; return insert(newpair, &idx); }
		bool insert( const Pair &newpair, jagint *index );
		// const Pair & operator[] ( jagint i ) const { return _arr[i]; }
		Pair & operator[] ( jagint i ) const { return _arr[i]; }


		bool exist( const Pair &pair ) const { jagint idx; return exist(pair, &idx); }
		bool exist( const Pair &pair, jagint *index ) const;

		bool remove( const Pair &pair, AbaxDestroyAction action=ABAX_NOOP ); 
		bool get( Pair &pair ) const; 
		Pair& get( const Pair &pair, bool &exist ) const; 
		bool set( const Pair &pair ); 
		/***
		Pair& at( jagint i ) const { 
			i = nextNonNull(i); return _arr[i]; 
		}
		***/
		void destroy();
		jagint size() const { return _arrlen; }
		const Pair* array() const { return (const Pair*)_arr; }
		jagint nextNonNull( jagint start) const { 
			if ( start < 0 ) start = 0;
			while ( start <= _arrlen-1 && _arr[start] == Pair::NULLVALUE ) ++start;
			return start; 
		}
		jagint prevNonNull( jagint start) const { 
			if ( start > _arrlen-1 ) start = _arrlen-1;
			while ( start >=0 && _arr[start] == Pair::NULLVALUE ) --start;
			return start; 
		}
		bool isNull( jagint i ) const { 
			if ( i < 0 || i >= _arrlen ) { return true; } return ( _arr[i] == Pair::NULLVALUE ); 
		}
		jagint elements() const { return _elements; }

		void printKeyStringOnly() const;
		void printKeyIntegerOnly() const;
		void printKeyStringValueString() const;
		void printKeyIntegerValueString() const;
		void printKeyStringValueInteger() const;
		void printKeyIntegerValueInteger() const;

	protected:

		void 	reAllocDistribute();
		void 	reAlloc();
		void 	reDistribute();

		void 	reAllocShrink();
		void 	reAllocDistributeShrink();
		void 	reDistributeShrink();

		jagint 	hashKey( const Pair &key, jagint arrlen ) const { 
			return key.hashCode() % arrlen; 
		}

    	jagint 	probeLocation( jagint hc, const Pair *arr, jagint arrlen ) const;
    	jagint 	findProbedLocation( const Pair &search, jagint hc ) const ;
    	void 	findCluster( jagint hc, jagint *start, jagint *end ) const;
    	jagint 	prevHC ( jagint hc, jagint arrlen ) const;
    	jagint 	nextHC( jagint hc, jagint arrlen ) const;
		jagint 	hashLocation( const Pair &pair, const Pair *arr, jagint arrlen ) const;
		void 	rehashCluster( jagint hc );
		bool 	aboveq( jagint start, jagint end, jagint birthhc, jagint nullbox ) const;

		Pair   		*_arr;
		jagint 		_arrlen;

		Pair   		*_newarr;
		jagint  	_newarrlen;

		jagint  	_elements;
		jagint		*_hashcol;
		jagint		*_newhashcol;
		Pair 		_min;
		Pair 		_max;
		Pair 		_dummy;

		//static const int LOAD_F1  = 1;	
		//static const int LOAD_F2  = 2;	
		// so load factor = F1/F2

		static const int _GEO  = 2;	 // fixed
		// static const int _defaultBase = 32;  

};

// ctor
template <class Pair> 
JagHashArray<Pair>::JagHashArray( int size )
{
	// _arr = new Pair[_defaultBase];
	_arr = new Pair[size];
	_arrlen = size;
	for ( jagint i =0; i <_arrlen; ++i) {
		_arr[i] = Pair::NULLVALUE;
	}

	_elements = 0;
}

// copy ctor
template <class Pair> 
JagHashArray<Pair>::JagHashArray( const JagHashArray<Pair> &arr2 )
{
	jagint size = arr2.size();
	_arr = new Pair[size];
	_arrlen = size;
	_elements = 0;
	for ( jagint i =0; i <_arrlen; ++i) {
		if ( ! arr2.isNull(i) ) {
			_arr[i] = arr2[i];
			++ _elements;
		} else {
			_arr[i] = Pair::NULLVALUE;
		}
	}
}

// dtor
template <class Pair> 
void JagHashArray<Pair>::destroy( )
{
	if ( _arr ) {
		delete [] _arr; 
	}
	_arr = NULL;
}

// dtor
template <class Pair> 
JagHashArray<Pair>::~JagHashArray( )
{
	destroy();
}

template <class Pair> 
void JagHashArray<Pair>::reAllocDistribute()
{
	// read lock: read can gointo, write cannot get into
	// printf("realloc ...\n");
	reAlloc();

	// printf("c3097 redistribute ...\n");
	reDistribute();
	// printf("redistribute done \n");

	// print();
	// release read lock
}

template <class Pair> 
void JagHashArray<Pair>::reAllocDistributeShrink()
{
	// read lock: read can gointo, write cannot get into
	reAllocShrink();
	reDistributeShrink();
	// release read lock
}

template <class Pair> 
void JagHashArray<Pair>::reAlloc()
{
	jagint i;
	_newarrlen = _GEO*_arrlen; 
	_newarr = new Pair[_newarrlen];

	for ( i=0; i < _newarrlen; ++i) {
		_newarr[i] = Pair::NULLVALUE;
	}
}

template <class Pair> 
void JagHashArray<Pair>::reAllocShrink()
{
	jagint i;
	_newarrlen  = _arrlen/_GEO; 
	_newarr = new Pair[_newarrlen];

	for ( i=0; i < _newarrlen; ++i) {
		_newarr[i] = Pair::NULLVALUE;
	}
}


template <class Pair> 
void JagHashArray<Pair>::reDistribute()
{
	jagint pos; 

	for ( jagint i = _arrlen-1; i>=0; --i) {
		if ( _arr[i] == Pair::NULLVALUE ) { continue; } 
		pos = hashLocation( _arr[i], _newarr, _newarrlen );
		_newarr[pos] = _arr[i];  // not null
	}

	// write lock
	if ( _arr ) {
		delete [] _arr;
	}
	_arrlen = _newarrlen;
	_arr = _newarr;

	// release write lock

}

template <class Pair> 
void JagHashArray<Pair>::reDistributeShrink()
{
	jagint pos; 

	for ( jagint i = _arrlen-1; i>=0; --i) {
		if ( _arr[i] == Pair::NULLVALUE ) { continue; } 
		pos = hashLocation( _arr[i], _newarr, _newarrlen );
		_newarr[pos] = _arr[i];
	}

	// write lock
	if ( _arr ) delete [] _arr;
	_arrlen = _newarrlen;
	_arr = _newarr;

	// release write lock
}


template <class Pair> 
bool JagHashArray<Pair>::insert( const Pair &newpair, jagint *retindex )
{
	bool rc;
	jagint index;

	// abaxcout << "Insert " << newpair << abaxendl;
	// printf("items=%lld  _arrlen=%lld index=%lld\n", _elements, _arrlen, index );
	if ( newpair == Pair::NULLVALUE ) { 
		// printf("s7013 insert NULLVALUE return 0\n"); fflush( stdout );
		return 0; 
	}

	rc = exist( newpair, retindex );
	if ( rc ) {
		// printf("s7014 pair exists return 0\n"); fflush( stdout );
		return false;
	}

	if ( ( _GEO*_elements ) >=  _arrlen-4 ) {
		reAllocDistribute();
	}

	// if ( 0 == _elements ) { /*_min = _max = newpair;*/ }

	index = hashLocation( newpair, _arr, _arrlen ); 
	_arr[index] = newpair;  
	++_elements;
	if ( _elements > 0 ) { 
		//if ( newpair < _min ) { _min = newpair; } else if ( newpair >_max ) { _max  = newpair; }
	}

	*retindex = index;
	return true;
}

template <class Pair> 
bool JagHashArray<Pair>::remove( const Pair &pair, AbaxDestroyAction action )
{
	jagint index;
	bool rc = exist( pair, &index );
	if ( ! rc ) return false;

	if ( action != ABAX_NOOP ) {
		_arr[index].valueDestroy( action ); 
	}

	_arr[index] = Pair::NULLVALUE;
	-- _elements;
	rehashCluster( index );

	if ( _arrlen >= 64 ) {
    	int loadfactor  = 100 * _elements / _arrlen;
    	if (  loadfactor < 20 ) {
    		reAllocDistributeShrink();
    	}
	} 

	return true;
}

template <class Pair> 
bool JagHashArray<Pair>::exist( const Pair &search, jagint *index ) const
{
    jagint idx = hashKey( search, _arrlen );
	//d("s2280 idx=hashcode=%ld\n", idx );
	*index = idx;
    
   	//  hash to NULL, not found.
    if ( _arr[idx] == Pair::NULLVALUE ) {
		//d("s0283 idx=%ld hash to NULL, not found\n", idx );
		//printKeyStringOnly();
   		return 0;
    }
       
    if ( search != _arr[idx] )  {
   		idx = findProbedLocation( search, idx );
   		if ( idx < 0 ) {
			//d("s7731 findProbedLocation idx=%ld < 0 return 0 \n", idx );
			//printKeyStringOnly();
   			return 0;
   		}
   	}
    
   	*index = idx;
    return 1;
}

template <class Pair> 
bool JagHashArray<Pair>::get( Pair &pair ) const
{
	jagint index;
	bool rc;

	rc = exist( pair, &index );
	if ( ! rc ) return false;

	pair.value = _arr[index].value;
	return true;
}

template <class Pair> 
Pair& JagHashArray<Pair>::get( const Pair &inpair, bool &exists ) const
{
	jagint index;
	exists = exist( inpair, &index );
	//d("s7209 this=%0x Pair& get exists=%d key=[%s]\n", this, exists, inpair.key.c_str() );
	if ( exists ) {
		return _arr[index];
	} else {
		return _arr[0];
		// return _dummy;
	}
}

template <class Pair> 
bool JagHashArray<Pair>::set( const Pair &pair )
{
	jagint index;
	bool rc;

	rc = exist( pair, &index );
	if ( ! rc ) return false;

	_arr[index].value = pair.value;
	return true;
}



template <class Pair> 
jagint JagHashArray<Pair>::hashLocation( const Pair &pair, const Pair *arr, jagint arrlen ) const
{
	jagint index = hashKey( pair, arrlen ); 
	if ( arr[index] != Pair::NULLVALUE ) {
		index = probeLocation( index, arr, arrlen );
	}
	return index;
}


template <class Pair> 
jagint JagHashArray<Pair>::probeLocation( jagint hc, const Pair *arr, jagint arrlen ) const
{
    while ( 1 ) {
    	hc = nextHC( hc, arrlen );
    	if ( arr[hc] == Pair::NULLVALUE ) { return hc; }
   	}
   	return -1;
}
    
// retrieve the previously assigned probed location
template <class Pair> 
jagint JagHashArray<Pair>::findProbedLocation( const Pair &search, jagint idx ) const
{
   	while ( 1 ) {
   		idx = nextHC( idx, _arrlen );
  
   		if ( _arr[idx] == Pair::NULLVALUE ) {
   			return -1; // no probed slot found
   		}
    
   		if ( search == _arr[idx] ) {
   			return idx;
   		}
   	}
   	return -1;
}
    
    
// find [start, end] of island that contains hc  
// [3, 9]   [12, 3]
template <class Pair> 
void JagHashArray<Pair>::findCluster( jagint hc, jagint *start, jagint *end ) const
{
	jagint i;
	i = hc;
  	// find start
   	while (1 ) {
   		i = prevHC( i, _arrlen );
 		if ( _arr[i] == Pair::NULLVALUE ) {
    		*start = nextHC( i, _arrlen );
    		break;
    	}
    }
    
    // find end
    i = hc;
    while (1 ) {
    	i = nextHC( i, _arrlen );
  
 		if ( _arr[i] == Pair::NULLVALUE ) {
    		*end = prevHC( i, _arrlen );
    		break;
    	}
   	}
}

    
template <class Pair> 
jagint JagHashArray<Pair>::prevHC ( jagint hc, jagint arrlen ) const
{
   	--hc; 
   	if ( hc < 0 ) hc = arrlen-1;
   	return hc;
}
    
template <class Pair> 
jagint JagHashArray<Pair>::nextHC( jagint hc, jagint arrlen ) const
{
 	++ hc;
   	if ( hc == arrlen ) hc = 0;
   	return hc;
}


template <class Pair> 
bool JagHashArray<Pair>::aboveq( jagint start, jagint end, jagint birthhc, jagint nullbox ) const
{
		if ( start < end ) {
			return ( birthhc <= nullbox ) ? true : false;
		} else {
			if ( 0 <= birthhc  && birthhc <= end ) {
				birthhc += _arrlen;
			}

			if ( 0 <= nullbox  && nullbox <= end ) {
				nullbox += _arrlen;
			}

			return ( birthhc <= nullbox ) ? true : false;
		}
}

template <class Pair> 
void JagHashArray<Pair>::rehashCluster( jagint hc )
{
	register jagint start, end;
	register jagint nullbox = hc;

	findCluster( hc, &start, &end );

	jagint  birthhc;
	bool b;

	while ( 1 )
	{
		hc = nextHC( hc, _arrlen );
		if  ( _arr[hc] == Pair::NULLVALUE ) {
			break;
		}

		birthhc = hashKey( _arr[hc], _arrlen );
		if ( birthhc == hc ) {
			continue;  // birth hc
		}

		b = aboveq( start, end, birthhc, nullbox );
		if ( b ) {
			_arr[nullbox] = _arr[hc];
			_arr[hc] = Pair::NULLVALUE;
			nullbox = hc;
		}
	}
}

template <class Pair> 
void JagHashArray<Pair>::printKeyStringOnly() const
{
	char buf[32];

	printf("JagHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i )
	{
		sprintf( buf, "%016lld", i );
		if ( _arr[i] != Pair::NULLVALUE ) {
			printf(" [%s]   [%s]\n", buf, _arr[i].key.c_str() );
		} else {
			printf(" [%s]   []\n", buf );
		}
	}
	printf("\n");
}

template <class Pair> 
void JagHashArray<Pair>::printKeyIntegerOnly() const
{
	char buf[16];

	printf("JagHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i )
	{
		sprintf( buf, "%08d", i );
		if ( _arr[i] != Pair::NULLVALUE ) {
			printf(" [%s]   [%lld]\n", buf, _arr[i].key.value() );
		}
	}
	printf("\n");
}

template <class Pair> 
void JagHashArray<Pair>::printKeyStringValueString() const
{
	char buf[16];

	printf("JagHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i)
	{
		sprintf( buf, "%08d", i );
		if ( _arr[i] != Pair::NULLVALUE ) {
			printf(" [%s]   [%s]   [%s]\n", buf, _arr[i].key.c_str(), _arr[i].value.key.c_str() );
		}
	}
	printf("\n");
}


template <class Pair> 
void JagHashArray<Pair>::printKeyIntegerValueString() const
{
	char buf[16];

	printf("JagHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i)
	{
		sprintf( buf, "%08d", i );
		if ( _arr[i] != Pair::NULLVALUE ) {
			printf(" %s   %lld   %s\n", buf, _arr[i].key.value(), _arr[i].value.key.c_str() );
		}
	}
	printf("\n");
}


template <class Pair> 
void JagHashArray<Pair>::printKeyStringValueInteger() const
{
	char buf[16];

	printf("JagHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i)
	{
		sprintf( buf, "%08d", i );
		if ( _arr[i] != Pair::NULLVALUE ) {
			printf(" %s   %s   %lld\n", buf, _arr[i].key.c_str(), _arr[i].value );
		}
	}
	printf("\n");
}

template <class Pair> 
void JagHashArray<Pair>::printKeyIntegerValueInteger() const
{
	char buf[16];

	printf("JagHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i)
	{
		sprintf( buf, "%08d", i );
		if ( _arr[i] != Pair::NULLVALUE ) {
			printf(" %s   %lld   %lld\n", buf, _arr[i].key.value(), _arr[i].value );
		}
	}
	printf("\n");
}

#endif
