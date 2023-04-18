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
#ifndef _jag_vector_class_h_
#define _jag_vector_class_h_

#include <abax.h>
#include <JagDef.h>

template <class Pair>
class JagGapVector
{
	public:

		JagGapVector( int initSize=128 );

		template <class P> JagGapVector( const JagGapVector<P>& other );
		JagGapVector<Pair>& operator=( const JagGapVector<Pair>& other );

		void init( int sz );
		~JagGapVector();

		// inline void update( jagint i, const Pair &pair ) { _arr[i] = pair; }
		inline const Pair & operator[] ( jagint i ) const { return _arr[i]; }
		inline Pair & operator[] ( jagint i ) { return _arr[i]; }
		inline void setNull( const Pair &pair, jagint i ) { 
			if ( pair != Pair::NULLVALUE && _arr[i] == pair ) {
				// abaxcout << "*** c5102 vec i=" << i << " setNull real set to NULL" << abaxendl;
				_arr[i] = Pair::NULLVALUE; 
				-- _elements; 
			}
		}

		bool isNull( jagint i ) 
		{
			if ( _arr[i] == Pair::NULLVALUE ) {
				return true;
			} else {
				return false;
			}
		}

		inline bool exist( const Pair &pair ) { jagint idx; return exist(pair, &idx); }
		bool 	exist( const Pair &pair, jagint *index );

		bool 	remove( const Pair &pair, AbaxDestroyAction action=ABAX_NOOP ); 
		bool 	get( Pair &pair ); 
		bool 	set( const Pair &pair ); 
		void 	destroy();
		inline  jagint capacity() const { return _arrlen; }
		inline  jagint size() const { return _elements; }
		inline  jagint last() const { return _last; }
		inline  const Pair* array() const { return (const Pair*)_arr; }
		// inline const Pair&  min() const { return _min; }
		// inline const Pair&  max() const { return _max; }
		inline bool append( const Pair &newpair ) { jagint idx; return append(newpair, &idx); }
		bool append( const Pair &newpair, jagint *index );
		// bool insert( const Pair &newpair, jagint index, bool force=0 );
		bool insertForce( const Pair &newpair, jagint index );
		bool insertLess( const Pair &newpair, jagint index );
		void setValue( int val, bool isSet, jagint index );
		bool findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset );
		inline bool setNull() {
			bool rc = false;
			if ( _elements > 0 ) {

				for ( jagint i = 0; i < _arrlen; ++i ) {
			    	_arr[i] = Pair::NULLVALUE;
				}	

				_elements = 0;
				_last = 0;
				rc = true;
			}
			return rc;
		}
		jagint getPartElements( jagint pos ) {
			if ( pos <= _last && _arr[pos].value.size() ) return *(_arr[pos].value.c_str());
			else return 0;
		}
		bool cleanPartPair( jagint pos ) {
			if( pos <= _last ) {
				_arr[pos] = Pair::NULLVALUE;
				return true;
			}
			return false;
		}

		bool deleteUpdateNeeded( const Pair &dpair, const Pair &npair, jagint pos ) {
			if( pos <= _last ) {
				if ( dpair <= _arr[pos] ) {
					_arr[pos] = npair;
            		return true;
        		}
    		}
    		return false;
		}

	void print()
		{
			printf("arrlen=%lld, elements=%lld, last=%lld\n", _arrlen, _elements, _last);
			for ( jagint i = 0; i <= _last; ++i ) {
				printf("i=%lld   [%s]\n", i, _arr[i].key.c_str() );
			}	
		}


		void 	reAlloc();
		void 	reAllocShrink();


	protected:

		Pair   		*_arr;
		jagint  	_arrlen;

		// temp vars
		Pair   		*_newarr;
		jagint  	_newarrlen;

		jagint  	_elements;
		jagint  	_last;
		// Pair 		_min;
		// Pair 		_max;

		static const int _GEO  = 2;	 // fixed

};

//////////////////////// vector code ///////////////////////////////////////////////
// ctor
template <class Pair> 
JagGapVector<Pair>::JagGapVector( int initSize )
{
	// printf("c7383 default ctor JagGapVector<Pair>::JagGapVector( int initSize ) ...\n");
	// fflush ( stdout );

	init( initSize );
}
template <class Pair> 
void JagGapVector<Pair>::init( int initSize )
{
	_arr = new Pair[initSize];
	_arrlen = initSize;
	_elements = 0;
	_last = 0;

	for ( jagint i = 0; i < _arrlen; ++i ) {
		_arr[i] = Pair::NULLVALUE;
	}
}

// copy ctor
template <class Pair>
template <class P>
JagGapVector<Pair>::JagGapVector( const JagGapVector<P>& other )
{
	// printf("c38484 JagGapVector<Pair>::JagGapVector( const JagGapVector<P>& other ) ...\n");
	// fflush ( stdout );

	_arrlen = other._arrlen;
	_elements = other._elements;
	_last = other._last;

	_arr = new Pair[_arrlen];
	for ( jagint i = 0; i < _arrlen; ++i ) {
		_arr[i] = other._arr[i];
	}
}


// assignment operator
template <class Pair>
JagGapVector<Pair>& JagGapVector<Pair>::operator=( const JagGapVector<Pair>& other )
{
	// printf("c8383 JagGapVector<Pair>& JagGapVector<Pair>::operator=() ...\n");
	// fflush ( stdout );
	if ( _arr == other._arr ) {
		return *this;
	}

	if ( _arr ) {
		delete [] _arr;
	}

	_arrlen = other._arrlen;
	_elements = other._elements;
	_last = other._last;

	_arr = new Pair[_arrlen];
	for ( jagint i = 0; i < _arrlen; ++i ) {
		_arr[i] = other._arr[i];
	}

	return *this;
}


// dtor
template <class Pair> 
void JagGapVector<Pair>::destroy( )
{
	if ( ! _arr ) {
		// printf("c6183 JagGapVector destroy _arr is NULL, return\n");
		// fflush( stdout );
		return;
	}

	if ( _arr ) {
		delete [] _arr; 
		_arr = NULL;
	}
}

// dtor
template <class Pair> 
JagGapVector<Pair>::~JagGapVector( )
{
	destroy();
}


template <class Pair> 
void JagGapVector<Pair>::reAlloc()
{
	jagint i, j;
	// _newarrlen  = _GEO*_arrlen; 
	// check to make sure arrlen is even and multiple of JAG_BLOCK_SIZE
	_newarrlen = _arrlen + _arrlen/2;
	j = _newarrlen % JAG_BLOCK_SIZE;
	_newarrlen += JAG_BLOCK_SIZE - j;

	_newarr = new Pair[_newarrlen];
	for ( i = 0; i < _arrlen; ++i) {
		_newarr[i] = _arr[i];
	}
	for ( i = _arrlen; i < _newarrlen; ++i) {
		_newarr[i] = Pair::NULLVALUE;
	}

	if ( _arr ) delete [] _arr;
	_arr = _newarr;
	_newarr = NULL;
	_arrlen = _newarrlen;
}

template <class Pair> 
void JagGapVector<Pair>::reAllocShrink()
{
	jagint i;

	_newarrlen  = _arrlen/_GEO; 

	_newarr = new Pair[_newarrlen];
	for ( i = 0; i < _elements; ++i) {
		_newarr[i] = _arr[i];
	}
	for ( i = _elements; i < _newarrlen; ++i) {
		_newarr[i] = Pair::NULLVALUE;
	}

	if ( _arr ) delete [] _arr;
	_arr = _newarr;
	_newarr = NULL;
	_arrlen = _newarrlen;
}


template <class Pair> 
bool JagGapVector<Pair>::append( const Pair &newpair, jagint *index )
{
	if ( _elements == _arrlen ) { reAlloc(); }
	*index = _elements;
	_arr[_elements++] = newpair;
	return true;
}

template <class Pair> 
bool JagGapVector<Pair>::insertForce( const Pair &newpair, jagint index )
{
	while ( index >= _arrlen ) { 
		reAlloc(); 
	}

	if ( _arr[index] == Pair::NULLVALUE ) {
		if ( newpair != Pair::NULLVALUE ) {
			++_elements;
			_arr[index] = newpair;
		}
	} else {
		if ( newpair == Pair::NULLVALUE ) {
			--_elements;
		}	
		_arr[index].key = newpair.key;
	}
	
	// _arr[index] = newpair;

	if ( index > _last ) {
		_last = index;
	}

	return true;
}

template <class Pair> 
bool JagGapVector<Pair>::insertLess( const Pair &newpair, jagint index )
{
	bool rc = false;
	while ( index >= _arrlen ) { 
		reAlloc(); 
	}

	if ( _arr[index] == Pair::NULLVALUE ) {
		++_elements;
		_arr[index] = newpair;
		rc = true;
	} else {
		if ( newpair < _arr[index] ) {
			_arr[index].key = newpair.key;
			rc = true;
		}
	}

	if ( index > _last ) {
		_last = index;
	}

	return rc;
}

template <class Pair>
void JagGapVector<Pair>::setValue( int val, bool isSet, jagint index )
{
	while ( index >= _arrlen ) { 
		reAlloc(); 
	}
	if ( !isSet ) {
		if ( _arr[index].value.size() ) {
			val += *(_arr[index].value.c_str());
		}
		if ( val < 0 ) val = 0;
		else if ( val > JAG_BLOCK_SIZE ) val = JAG_BLOCK_SIZE;
	}
	char buf[2];
	char c = val;
	buf[0] = c;
	buf[1] = '\0';
	JagFixString value( buf, 1);
	_arr[index].value = value;
}		

template <class Pair>
bool JagGapVector<Pair>::findLimitStart( jagint &startlen, jagint limitstart, jagint &soffset )
{
	bool isEnd = false;
	for ( jagint i = 0; i < _arrlen; ++i ) {
		if ( _arr[i].value.size() ) {
			startlen += *(_arr[i].value.c_str());
		}

		if ( startlen >= limitstart ) {
			isEnd = true;
			if ( _arr[i].value.size() ) {
				startlen -= *(_arr[i].value.c_str());
			}
			soffset = i;
			break;
		}
	}
	// printf("s9999 end soffset=%lld, startlen=%lld\n", soffset, startlen);
	return isEnd;
}

// Slow, should not use this
template <class Pair> 
bool JagGapVector<Pair>::remove( const Pair &pair, AbaxDestroyAction action )
{
	jagint i, index;
	bool rc = exist( pair, &index );
	if ( ! rc ) return false;

	if ( action != ABAX_NOOP ) {
		_arr[index].valueDestroy( action ); 
	}

	// shift left
	for ( i = index; i <= _elements-2; ++i ) {
		_arr[i] = _arr[i+1];
	}
	-- _elements;

	if ( _arrlen >= 64 ) {
    	jagint loadfactor  = 100 * (jagint)_elements / _arrlen;
    	if (  loadfactor < 15 ) {
    		reAllocShrink();
    	}
	} 

	if ( index == _last ) {
		-- _last;
	}

	return true;
}


/**********
// scan search  slow
template <class Pair> 
bool JagGapVector<Pair>::exist( const Pair &search, jagint *index )
{
    jagint i; 

	for ( i = 0; i < _elements; ++i ) {
		if ( _arr[i] == search ) {
			*index = i;
			return true;
		}
	}
    
    return 0;
}

template <class Pair> 
inline bool JagGapVector<Pair>::get( Pair &pair )
{
	jagint index;
	bool rc;

	rc = exist( pair, &index );
	if ( ! rc ) return false;

	pair.value = _arr[index].value;
	return true;
}

template <class Pair> 
inline bool JagGapVector<Pair>::set( const Pair &pair )
{
	jagint index;
	bool rc;

	rc = exist( pair, &index );
	if ( ! rc ) return false;

	_arr[index].value = pair.value;
	return true;
}

***********/

#endif
