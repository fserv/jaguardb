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
#ifndef _jag_stack_h_
#define _jag_stack_h_


template <class Pair>
class JagStack
{
	public:
		JagStack( int initSize = 4 );
		JagStack( const JagStack<Pair> &str );
		JagStack<Pair>& operator=( const JagStack<Pair> &s2 );
		~JagStack();

		void	clean( int initSize = 4 );
		void 	push( const Pair &pair );
		void 	pop();
		const   Pair &top() const;
		const   Pair & operator[](int i) const;
		inline  jagint size() const { return _last+1; }
		inline  jagint capacity() const { return _arrlen; }
		void 	destroy();
		void 	print();
		void 	reAlloc();
		void 	reAllocShrink();
		void    concurrent( bool flag = true );
		inline bool    empty() { if ( _last >=0 ) return false; else return true; }

	protected:
		Pair   		*_arr;
		jagint  	_arrlen;
		jagint  	_last;
		static const int _GEO  = 2;
		bool        _isDestroyed;
};

// ctor
template <class Pair> 
JagStack<Pair>::JagStack( int initSize )
{
	//_arr = new Pair[initSize];
	_arr = (Pair*)calloc( initSize, sizeof( Pair ) );
	memset(_arr, initSize*sizeof(Pair), 0 );
	_arrlen = initSize;
	_last = -1;
	_isDestroyed = false;

}

// copy ctor
template <class Pair> 
JagStack<Pair>::JagStack( const JagStack<Pair> &str )
{
	throw 2345;
	if ( _arr == str._arr ) return;
	if ( _arr ) free( _arr );

	_arrlen = str._arrlen;
	_last = str._last;
	_arr = (Pair*)calloc( _arrlen, sizeof( Pair ) ); 
	for ( int i = 0; i < _arrlen; ++i ) {
		_arr[i] = str._arr[i];
	}
	_isDestroyed = false;
}

// assignment operator
template <class Pair> 
JagStack<Pair>& JagStack<Pair>::operator=( const JagStack<Pair> &str )
{
	throw 2346;
	if ( _arr == str._arr ) return *this;
	if ( _arr ) free( _arr );

	_arrlen = str._arrlen;
	_last = str._last;
	_arr = (Pair*)calloc( _arrlen, sizeof( Pair ) ); 
	for ( int i = 0; i < _arrlen; ++i ) {
		_arr[i] = str._arr[i];
	}
	_isDestroyed = false;
	return *this;
}

// dtor
template <class Pair> 
void JagStack<Pair>::destroy( )
{
	//d("s2727 this=%0x in destroy _arr=%0x _isDestroyed=%d\n", this, _arr, _isDestroyed );
	if ( ! _arr ) {
		return;
	}
	if ( _isDestroyed ) {
		return;
	}

	if ( _last < 0 ) {
		if ( _arr ) free ( _arr );
		_arrlen = 0;
		_last = -1;
		_arr = NULL;
		return;
	}

	if ( _arr ) free ( _arr );
	_arrlen = 0;
	_last = -1;
	_arr = NULL;
	_isDestroyed = true;
}

// dtor
template <class Pair> 
JagStack<Pair>::~JagStack( )
{
	destroy();
}

template <class Pair>
void JagStack<Pair>::clean( int initSize )
{
	//d("s8282 stack cleaned this=%0x\n", this );
	destroy();
}

template <class Pair> 
void JagStack<Pair>::reAlloc()
{
	jagint i;
	jagint newarrlen  = _GEO*_arrlen; 
	Pair *newarr;

	// newarr = new Pair[newarrlen];
	newarr = (Pair*)calloc( newarrlen, sizeof( Pair ) );
	for ( i = 0; i <= _last; ++i) {
		newarr[i] = _arr[i];
	}
	if ( _arr ) free( _arr );
	_arr = newarr;
	newarr = NULL;
	_arrlen = newarrlen;
}

template <class Pair> 
void JagStack<Pair>::reAllocShrink()
{
	jagint i;
	Pair *newarr;

	jagint newarrlen  = _arrlen/_GEO; 
	newarr = (Pair*)calloc( newarrlen, sizeof( Pair ) ); 
	for ( i = 0; i <= _last; ++i) {
		newarr[i] = _arr[i];
	}

	if ( _arr ) free( _arr );
	_arr = newarr;
	newarr = NULL;
	_arrlen = newarrlen;
}

template <class Pair> 
void JagStack<Pair>::push( const Pair &newpair )
{
	if ( NULL == _arr ) {
		_arr = (Pair*)calloc( 4, sizeof( Pair ) );
		memset(_arr, 4*sizeof(Pair), 0 );
		_arrlen = 4;
		_last = -1;
	}

	if ( _last == _arrlen-1 ) { reAlloc(); }

	++ _last;
	_arr[_last] = newpair;

}

// back: add end (enqueue end)
template <class Pair> 
const Pair &JagStack<Pair>::top() const
{
	if ( _last < 0 ) {
		printf("s5004 stack empty, error top()\n");
		throw 2920;
	} 
	return _arr[ _last ];
}


template <class Pair> 
void JagStack<Pair>::pop()
{
	if ( _last < 0 ) {
		return;
	} 

	if ( _arrlen >= 64 ) {
    	jagint loadfactor  = (100 * _last) / _arrlen;
    	if (  loadfactor < 20 ) {
    		reAllocShrink();
    	}
	} 

	-- _last;
}


template <class Pair> 
void JagStack<Pair>::print()
{
	jagint i;
	printf("c3012 JagStack this=%0x _arrlen=%d _last=%d \n", this, _arrlen, _last );
	for ( i = 0; i  <= _last; ++i) {
		printf("%09d  %0x\n", i, _arr[i] );
	}
	printf("\n");
}

template <class Pair> 
const Pair& JagStack<Pair>::operator[](int i) const 
{ 
	if ( i<0 ) { i=0; }
	else if ( i > _last ) { i = _last; }
	return _arr[i];
}

#endif
