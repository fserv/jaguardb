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
#ifndef _jag_dbpair_h_
#define _jag_dbpair_h_

#include <math.h>
#include "JagHashMap.h"
#include "JagSchemaRecord.h"

class JagDBPair
{

    public:
        JagFixString  key;
        JagFixString  value;
		static  JagDBPair  NULLVALUE;
		const JagSchemaRecord  *_tabRec;
		
		JagDBPair() { _tabRec=NULL; } 
        JagDBPair( const JagFixString &k ) : key(k) { _tabRec=NULL; }
        JagDBPair( const JagFixString &k, const JagFixString &v ) : key(k), value(v) { _tabRec=NULL; } 

		void point( const JagFixString &k, const JagFixString &v ) 
		{
			key.point(k.addr(), k.size() );
			value.point( v.addr(), v.size() );
			_tabRec=NULL;
		}

		void point( const JagFixString &k ) 
		{
			key.point(k.addr(), k.size() );
			_tabRec=NULL;
		}

		void point( const char *k, jagint klen ) 
		{
			key.point( k, klen);
			_tabRec=NULL;
		}

        JagDBPair( const JagFixString &k, const JagFixString &v, bool ref ) 
		{
			key.point(k.addr(), k.size() );
			value.point( v.addr(), v.size() );
			_tabRec=NULL;
		}

        JagDBPair( const JagFixString &k, bool point ) 
		{
			key.point(k.addr(), k.size() );
			_tabRec=NULL;
		}

        JagDBPair( const char *kstr, jagint klen, const char *vstr, jagint vlen ) 
		{
			key = JagFixString(kstr, klen, klen);
			value = JagFixString(vstr, vlen, vlen);
			_tabRec=NULL;
		}

        JagDBPair( const char *kvstr, jagint klen, jagint vlen ) 
		{
			key = JagFixString(kvstr, klen, klen );
			value = JagFixString(kvstr+klen, vlen, vlen );
			_tabRec=NULL;
		}

		void point( const char *k, jagint klen,  const char *v, jagint vlen ) {
			key.point( k, klen);
			value.point( v, vlen );
			_tabRec=NULL;
		}

        JagDBPair( const char *kstr, jagint klen, const char *vstr, jagint vlen, bool point ) 
		{
			key.point(kstr, klen);
			value.point(vstr, vlen);
			_tabRec=NULL;
		}

        JagDBPair( const char *kstr, jagint klen ) 
		{
			key = JagFixString(kstr, klen, klen );
			_tabRec=NULL;
		}

        JagDBPair( const char *kstr, jagint klen, bool point ) 
		{
			key.point(kstr, klen);
			_tabRec=NULL;
		}

		void setTabRec( const JagSchemaRecord *tabrec )
		{
			_tabRec = tabrec;
		}

		// Return  0: if this and d2 is equal
		//         -1: if this is < d2
		//         +1: if this is > d2
        int compareKeys( const JagDBPair &d2 ) const;
		int compareByRec( const char *first, const char *second ) const;

		// operators
        int operator <( const JagDBPair &d2 ) const 
        {
    		return ( compareKeys( d2 ) < 0 ? 1:0 );
		}

        int operator <=( const JagDBPair &d2 ) const 
        {
    		return ( compareKeys( d2 ) <= 0 ? 1:0 );
		}
		
        int operator >( const JagDBPair &d2 ) const 
        {
    		return ( compareKeys( d2 ) > 0 ? 1:0 );
		}

        int operator >=( const JagDBPair &d2 ) const
        {
    		return ( compareKeys( d2 ) >= 0 ? 1:0 );
        }

        int operator ==( const JagDBPair &d2 ) const
        {
    		return ( compareKeys( d2 ) == 0 ? 1:0 );
        }

        int operator !=( const JagDBPair &d2 ) const
        {
    		return ( ! compareKeys( d2 ) == 0 ? 1:0 );
        }

        JagDBPair&  operator= ( const JagDBPair &d3 )
        {
            key = d3.key;
            value = d3.value;
            return *this;
        }

		void valueDestroy( AbaxDestroyAction action ) 
        {
			//value.destroy( action );
			value.destroy();
		}

		jagint hashCode() const 
        {
			return key.hashCode();
		}

		jagint size() 
        {
			return (key.size() + value.size()); 
		}

		void print() const;
		void printkv(bool newline=true) const;
		void p(bool newline=true) const;
		void toBuffer(char *buffer) const;
        void printColumns() const;
        void printColumns( const char *buf ) const;

		// caller must free it
		char *newBuffer() const;
};

#endif
