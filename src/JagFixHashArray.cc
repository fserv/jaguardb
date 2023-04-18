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

#include <JagFixHashArray.h>

#define NBT  '\0'
JagFixHashArray::JagFixHashArray( int inklen, int invlen )
{
	klen = inklen;
	vlen = invlen;
	kvlen = klen + vlen;
	jagint size = 256;
	_arr = jagmalloc(size*kvlen);
	_arrlen = size;
	memset( _arr, 0, _arrlen*kvlen );
	_elements = 0;
}

void JagFixHashArray::destroy( )
{
	if ( _arr ) {
		free(_arr); 
	}
	_arr = NULL;
}

JagFixHashArray::~JagFixHashArray( )
{
	destroy();
}

void JagFixHashArray::removeAll()
{
	destroy();

	jagint size = 256;
	_arr = jagmalloc(size*kvlen);
	_arrlen = size;
	memset( _arr, 0, _arrlen*kvlen );
	_elements = 0;
}

void JagFixHashArray::reAllocDistribute()
{
	reAlloc();
	reDistribute();
}

void JagFixHashArray::reAllocDistributeShrink()
{
	reAllocShrink();
	reDistributeShrink();
}

void JagFixHashArray::reAlloc()
{
	jagint i;
	_newarrlen = _GEO*_arrlen; 
	_newarr = jagmalloc(_newarrlen*kvlen);

	for ( i=0; i < _newarrlen; ++i) {
		_newarr[i*kvlen] = NBT;
	}
}

void JagFixHashArray::reAllocShrink()
{
	jagint i;
	_newarrlen  = _arrlen/_GEO; 
	_newarr = jagmalloc(_newarrlen*kvlen);

	for ( i=0; i < _newarrlen; ++i) {
		_newarr[i*kvlen] = NBT;
	}
}

void JagFixHashArray::reDistribute()
{
	jagint pos; 

	for ( jagint i = _arrlen-1; i>=0; --i) {
		if ( _arr[i*kvlen] == NBT ) { continue; } 
		pos = hashLocation( _arr+i*kvlen, _newarr, _newarrlen );
		memcpy( _newarr+pos*kvlen, _arr+i*kvlen, kvlen );
	}

	if ( _arr ) {
		free(_arr);
	}
	_arrlen = _newarrlen;
	_arr = _newarr;
}

void JagFixHashArray::reDistributeShrink()
{
	jagint pos; 

	for ( jagint i = _arrlen-1; i>=0; --i) {
		if ( _arr[i*kvlen] == NBT ) { continue; } 
		pos = hashLocation( _arr+i*kvlen, _newarr, _newarrlen );
		memcpy( _newarr+pos*kvlen, _arr+i*kvlen, kvlen );
	}

	if ( _arr ) free(_arr);
	_arrlen = _newarrlen;
	_arr = _newarr;
}

bool JagFixHashArray::insert( const char *newpair )
{
	bool rc;
	jagint index;

	if ( *newpair == NBT  ) { 
		return 0; 
	}

	jagint retindex;
	rc = exist( newpair, &retindex );
	if ( rc ) {
		return false;
	}

	if ( ( _GEO*_elements ) >=  _arrlen-4 ) {
		reAllocDistribute();
	}

	index = hashLocation( newpair, _arr, _arrlen ); 
	memcpy( _arr+index*kvlen, newpair, kvlen );
	++_elements;

	return true;
}

bool JagFixHashArray::remove( const char *pair )
{
	jagint index;
	bool rc = exist( pair, &index );
	if ( ! rc ) return false;

	rehashCluster( index );
	-- _elements;

	if ( _arrlen >= 64 ) {
    	int loadfactor  = 100 * _elements / _arrlen;
    	if (  loadfactor < 20 ) {
    		reAllocDistributeShrink();
    	}
	} 

	return true;
}

bool JagFixHashArray::exist( const char *search, jagint *index )
{
    jagint idx = hashKey( search, _arrlen );
	*index = idx;
    
    if ( _arr[idx*kvlen] == NBT ) {
   		return 0;
    }
       
    if ( 0 != memcmp( search, _arr+idx*kvlen, klen ) ) {
   		idx = findProbedLocation( search, idx );
   		if ( idx < 0 ) {
   			return 0;
   		}
   	}
    
   	*index = idx;
    return 1;
}

bool JagFixHashArray::get(  char *pair )
{
	jagint index;
	bool rc;

	rc = exist( pair, &index );
	if ( ! rc ) return false;

	memcpy( pair+klen, _arr+index*kvlen+klen, vlen ); 
	return true;
}

bool JagFixHashArray::get( const char *key, char *val )
{
	jagint index;
	bool rc;

	rc = exist( key, &index );
	if ( ! rc ) return false;

	memcpy( val, _arr+index*kvlen+klen, vlen ); 
	return true;
}

bool JagFixHashArray::set( const char *pair )
{
	jagint index;
	bool rc;

	rc = exist( pair, &index );
	if ( ! rc ) return false;

	memcpy( _arr+index*kvlen+klen,  pair+klen, vlen ); 
	return true;
}

jagint JagFixHashArray::hashLocation( const char *pair, const char *arr, jagint arrlen )
{
	jagint index = hashKey( pair, arrlen ); 
	if ( arr[index*kvlen] != NBT ) {
		index = probeLocation( index, arr, arrlen );
	}
	return index;
}


jagint JagFixHashArray::probeLocation( jagint hc, const char *arr, jagint arrlen )
{
    while ( 1 ) {
    	hc = nextHC( hc, arrlen );
    	if ( arr[hc*kvlen] == NBT ) { return hc; }
   	}
   	return -1;
}
    
jagint JagFixHashArray::findProbedLocation( const char *search, jagint idx ) 
{
   	while ( 1 ) {
   		idx = nextHC( idx, _arrlen );
  
   		if ( _arr[idx*kvlen] == NBT ) {
   			return -1; 
   		}
    
   		if ( 0==memcmp( search, _arr+idx*kvlen, klen) ) {
   			return idx;
   		}
   	}
   	return -1;
}
    
inline void JagFixHashArray::findCluster( jagint hc, jagint *start, jagint *end )
{
	jagint i;
	i = hc;
   	while (1 ) {
   		i = prevHC( i, _arrlen );
 		if ( _arr[i*kvlen] == NBT ) {
    		*start = nextHC( i, _arrlen );
    		break;
    	}
    }
    
    i = hc;
    while (1 ) {
    	i = nextHC( i, _arrlen );
  
 		if ( _arr[i*kvlen] == NBT ) {
    		*end = prevHC( i, _arrlen );
    		break;
    	}
   	}
}

    
inline jagint JagFixHashArray::prevHC ( jagint hc, jagint arrlen )
{
   	--hc; 
   	if ( hc < 0 ) hc = arrlen-1;
   	return hc;
}
    
inline jagint JagFixHashArray::nextHC( jagint hc, jagint arrlen )
{
 	++ hc;
   	if ( hc == arrlen ) hc = 0;
   	return hc;
}


bool JagFixHashArray::aboveq( jagint start, jagint end, jagint birthhc, jagint nullbox )
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

void JagFixHashArray::rehashCluster( jagint hc )
{
	register jagint start, end;
	register jagint nullbox = hc;

	findCluster( hc, &start, &end );
	_arr[ hc*kvlen] = NBT;


	jagint  birthhc;
	bool b;

	while ( 1 )
	{
		hc = nextHC( hc, _arrlen );
		if  ( _arr[hc*kvlen] == NBT ) {
			break;
		}

		birthhc = hashKey( _arr+hc*kvlen, _arrlen );
		if ( birthhc == hc ) {
			continue;  
		}

		b = aboveq( start, end, birthhc, nullbox );
		if ( b ) {
			memcpy( _arr+nullbox*kvlen, _arr+hc*kvlen, kvlen );
			_arr[hc*kvlen] = NBT;
			nullbox = hc;
		}
	}
}

    
void JagFixHashArray::print() const
{
	char buf[32];

	printf("JagFixHashArray: _arrlen=%lld _elements=%lld\n", _arrlen, _elements );
	for ( jagint i=0; i < _arrlen; ++i)
	{
		sprintf( buf, "%08lld", i );
		if ( _arr[i*kvlen] != NBT ) {
			printf(" %s   ", buf );
			for (int k=0; k <klen; ++k ) {
				printf("%c", *(_arr+i*kvlen+k) );
			}
			printf("\n");
		} 
	}
	printf("\n");
}

jagint JagFixHashArray::hashKey( const char *key, jagint arrlen ) const
{
	unsigned int hash[4];
	unsigned int seed = 42;
	MurmurHash3_x64_128( (void*)key, klen, seed, hash);
	uint64_t res = ((uint64_t*)hash)[0];
	jagint rc = res % arrlen;
	return rc;
}

bool JagFixHashArray::isNull( jagint i ) const 
{
	if ( i < 0 || i >= _arrlen ) { return true; } 
	return ( _arr[i*kvlen] == NBT ); 
}

