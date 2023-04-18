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
#ifndef _jag_fixhasharr_h_
#define _jag_fixhasharr_h_
#include <stdio.h>
#include <abax.h>
#include <JagUtil.h>

///////////////////////////////// fix hash array class ///////////////////////////////////
class JagFixHashArray
{
	public:
		int klen;
		int vlen;
		int kvlen;

		JagFixHashArray( int klen, int vlen );
		~JagFixHashArray();

		bool insert( const char* newpair );
		bool exist( const char *pair, jagint *index );
		bool remove( const char* pair );
		void removeAll();
		bool get( char *pair ); 
		bool get( const char *key, char *val ); 
		bool set( const char *pair ); 
		void destroy();
		jagint size() const { return _arrlen; }
		jagint arrayLength() const { return _arrlen; }
		const char* array() const { return (const char*)_arr; }
		jagint elements() { return _elements; }
		bool isNull( jagint i ) const;
		void print() const;

	protected:

		void 	reAllocDistribute();
		void 	reAlloc();
		void 	reDistribute();

		void 	reAllocShrink();
		void 	reAllocDistributeShrink();
		void 	reDistributeShrink();

		jagint	hashKey( const char *key, jagint arrlen ) const;
    	inline jagint 	probeLocation( jagint hc, const char *arr, jagint arrlen );
    	inline jagint 	findProbedLocation( const char *search, jagint hc ) ;
    	inline void 	findCluster( jagint hc, jagint *start, jagint *end );
    	inline jagint 	prevHC ( jagint hc, jagint arrlen );
    	inline jagint 	nextHC( jagint hc, jagint arrlen );
		inline jagint  hashLocation( const char *pair, const char *arr, jagint arrlen );
		inline void 	rehashCluster( jagint hc );
		inline bool 	aboveq( jagint start, jagint end, jagint birthhc, jagint nullbox );

		char   		*_arr;
		jagint  	 _arrlen;
		char   		*_newarr;
		jagint  	 _newarrlen;
		jagint  	    _elements;
		static const int _GEO  = 2;	 // fixed
};


#endif
