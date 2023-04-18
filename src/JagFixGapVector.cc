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

#include <JagFixGapVector.h>
#include <JagUtil.h>

JagFixGapVector::JagFixGapVector()
{
}
void JagFixGapVector::initWithKlen( int inklen )
{
	klen  = inklen;
	vlen  = 1;
	kvlen = klen + vlen;
	init( 32 );
}

JagFixGapVector::JagFixGapVector( int inklen )
{
	klen  = inklen;
	vlen  = 1;
	kvlen = klen + vlen;
	init( 32 );
}
void JagFixGapVector::init( int initSize )
{
	_arr = jagmalloc( initSize*kvlen );
	memset( (void*)_arr, 0, initSize*kvlen ); 
	_arrlen = initSize;
	_elements = 0;
	_last = 0;

}

void JagFixGapVector::destroy( )
{
	if ( ! _arr ) {
		return;
	}

	if ( _arr ) {
		free( _arr );
	}
	_arr = NULL;
}

JagFixGapVector::~JagFixGapVector( )
{
	destroy();
}

void JagFixGapVector::reAlloc()
{
	jagint i, j;
	_newarrlen = _arrlen + _arrlen/2;
	j = _newarrlen % JAG_BLOCK_SIZE;
	_newarrlen += JAG_BLOCK_SIZE - j;

	_newarr = jagmalloc( _newarrlen*kvlen );
	memcpy( _newarr, _arr, _arrlen*kvlen );

	for ( i = _arrlen; i < _newarrlen; ++i) {
		_newarr[i*kvlen] = NBT;
		_newarr[i*kvlen+klen] = 0;
	}

	if ( _arr ) free( _arr );
	_arr = _newarr;
	_newarr = NULL;
	_arrlen = _newarrlen;
}

void JagFixGapVector::reAllocShrink()
{
	jagint i;

	_newarrlen  = _arrlen/_GEO; 

	_newarr = jagmalloc( _newarrlen*kvlen );
	memcpy( _newarr, _arr, _elements*kvlen );
	for ( i = _elements; i < _newarrlen; ++i) {
		_newarr[i*kvlen] = NBT;
		_newarr[i*kvlen+klen] = 0;
	}

	if ( _arr ) free(_arr);
	_arr = _newarr;
	_newarr = NULL;
	_arrlen = _newarrlen;
}

bool JagFixGapVector::insertForce( const char *newpair, jagint index )
{
	while ( index >= _arrlen ) { 
		reAlloc(); 
	}

	if ( _arr[index*kvlen] == NBT ) {
		if ( *newpair != NBT ) {
			++_elements;
			memcpy(_arr + index*kvlen, newpair, klen );
		 	_arr[index*kvlen+klen] = 0;
		}
	} else {
		if ( *newpair == NBT ) {
			--_elements;
		}	
		memcpy(_arr + index*kvlen, newpair, klen );
	}
	
	if ( index > _last ) {
		_last = index;
	}

	return true;
}

bool JagFixGapVector::insertLess( const char *newpair, jagint index )
{
	bool rc = false;
	while ( index >= _arrlen ) { 
		reAlloc(); 
	}

	if ( _arr[index*kvlen] == NBT ) {
		++_elements;
		memcpy(_arr+index*kvlen, newpair, klen );
		 _arr[index*kvlen+klen] = 0;
		rc = true;
	} else {
		if ( memcmp(newpair, _arr+index*kvlen, klen) < 0 ) {
			memcpy(_arr+index*kvlen, newpair, klen );
			rc = true;
		}
	}

	if ( index > _last ) {
		_last = index;
	}

	return rc;
}

void JagFixGapVector::setValue( int val, bool isSet, jagint index )
{
	while ( index >= _arrlen ) { 
		reAlloc(); 
	}

	if ( !isSet ) {
		if ( _arr[index*kvlen+klen] > 0 ) {
			val += _arr[index*kvlen+klen];
		} else {
		}

		if ( val < 0 ) val = 0;
		else if ( val > JAG_BLOCK_SIZE ) val = JAG_BLOCK_SIZE;
	}

	_arr[index*kvlen+klen] = val;
}		

bool JagFixGapVector::findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset )
{
	bool isEnd = false;
	int v;
	for ( jagint i = 0; i < _arrlen; ++i ) {
		v = _arr[i*kvlen+klen];
		if (  v > 0 ) {
			startlen += v;
		}

		if ( startlen >= limitstart ) {
			isEnd = true;
			if ( v > 0 ) {
				startlen -= v;
			}
			soffset = i;
			break;
		}
	}
	return isEnd;
}

bool JagFixGapVector::setNull() 
{
	bool rc = false;
	if ( _elements > 0 ) {
		for ( jagint i = 0; i < _arrlen; ++i ) {
	    	_arr[i*kvlen] = NBT;
		}	

		_elements = 0;
		_last = 0;
		rc = true;
	}
	return rc;
}

void JagFixGapVector::setNull( const char *pair, jagint i )
{
	if ( *pair != NBT && 0==memcmp(_arr+i*kvlen, pair, klen) ) {
		_arr[i*kvlen] = NBT;
		-- _elements; 
	}
}

bool JagFixGapVector::isNull( jagint i )  const
{
	if ( _arr[i*kvlen] == NBT ) {
		return true;
	} else {
		return false;
	}
}

jagint JagFixGapVector::getPartElements( jagint pos )  const
{
	if ( pos <= _last && _arr[pos*kvlen+klen] > 0 ) return _arr[pos*kvlen+klen];
	else return 0;
}

bool JagFixGapVector::cleanPartPair( jagint pos ) 
{
	if( pos <= _last ) {
		_arr[pos*kvlen] = NBT;
		_arr[pos*kvlen+klen] = 0;
		--_elements;
		return true;
	}
	return false;
}

bool JagFixGapVector::deleteUpdateNeeded( const char *dpair, const char *npair, jagint pos ) 
{
	if( pos <= _last ) {
		if ( memcmp(dpair, _arr+pos*kvlen, klen) <= 0 ) {
			if ( npair ) {
				memcpy(_arr+pos*kvlen, npair, klen );
			} else {
				_arr[pos*kvlen] = NBT;
				_arr[pos*kvlen+klen] = 1; 
				--_elements;
			}
			return true;
		}
	}
	return false;
}

void JagFixGapVector::print() const
{
	printf("arrlen=%lld, elements=%lld, last=%lld\n", _arrlen, _elements, _last);
	for ( jagint i = 0; i <= _last; ++i ) {
		printf("i=%lld   \n", i  );
		for ( int j=0; j < klen; ++j ) {
			if ( _arr[i*kvlen] == NBT ) break;
			printf("%c", *(_arr+i*kvlen+j) );
		}
		printf("-%d", *(_arr+i*kvlen+klen) );
		printf("\n"  );
	}	
}

