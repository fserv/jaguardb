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
#ifndef _jag_simple_bounded_arrray_
#define _jag_simple_bounded_arrray_

template <class Pair>
class JagSimpleBoundedArray
{
    public:
		JagSimpleBoundedArray( jagint limit );
		~JagSimpleBoundedArray();

		void insert( const Pair &newpair ); 
		Pair& at( jagint i ) const { return _arr[i]; }
		void destroy() { delete [] _arr; }
		void clean() { destroy(); init(); }
		bool   isNull( jagint i ) const { return _arr[i] == Pair::NULLVALUE; }
		void   init();

  		Pair    *_arr;
		jagint _limit;
		jagint _nextempty;
		bool    _hasElement;
		
		// iterator
		void begin();
		bool next( Pair &pair );
		jagint _iterptr;
		bool    _atend;
};

template <class Pair>
JagSimpleBoundedArray<Pair>::JagSimpleBoundedArray( jagint limit )
{
	_limit = limit;
	init();
}

template <class Pair>
void JagSimpleBoundedArray<Pair>::init( )
{
	_arr = new Pair[_limit];
	for ( jagint i = 0; i < _limit; ++i ) {
		_arr[i] = Pair::NULLVALUE;
	}
	_nextempty = 0;
	_hasElement = 0;
}

template <class Pair>
JagSimpleBoundedArray<Pair>::~JagSimpleBoundedArray( )
{
	delete [] _arr;
}

template <class Pair>
void JagSimpleBoundedArray<Pair>::insert( const Pair &newpair ) 
{ 
	_arr[_nextempty] = newpair;
	_nextempty = (_nextempty+1) % _limit;
	_hasElement = 1;
}


// iteration methods
template <class Pair>
void JagSimpleBoundedArray<Pair>::begin()
{
	_iterptr = _nextempty;
	if ( isNull( _nextempty ) ) {
		_iterptr = 0;
	}

	_atend = 0;
}

template <class Pair>
bool JagSimpleBoundedArray<Pair>::next( Pair &pair )
{
	if ( _atend || 0==_hasElement ) {
		return false;
	}

	pair = _arr[_iterptr];
	_iterptr = (_iterptr+1) % _limit;

	if ( _iterptr == _nextempty || isNull(_iterptr)  ) {
		_atend = 1;
	}

	return 1;
}

#endif
