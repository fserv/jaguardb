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
#ifndef _jag_famkeychecker_h_
#define _jag_famkeychecker_h_
#include <stdio.h>
#include <abax.h>
#include <JagDef.h>
#include <JagMutex.h>

class JagFamilyKeyChecker 
{
    public: 
        JagFamilyKeyChecker( const Jstr &fpath, int klen, int vlen );
        virtual ~JagFamilyKeyChecker();
        virtual bool addKeyValue( const char *kv ) = 0;
        virtual bool addKeyValueNoLock( const char *kv ) = 0;
        virtual bool addKeyValueInit( const char *kv ) = 0;
        virtual bool getValue( const char *key, char *val ) = 0;
        virtual bool removeKey( const char *key ) = 0;
        virtual bool exist( const char *key ) const = 0;
		virtual void removeAllKey( ) = 0;
        virtual jagint size() const = 0;
        virtual jagint arrayLength() const = 0;
		virtual const char *array() const = 0;
		virtual int buildInitKeyCheckerFromSigFile() = 0;
		Jstr getPath() { return _pathName; }

    protected:
		void  getUniqueKey( const char *key, char *ukey ) const;
		int   _KLEN;
		int   _VLEN;
		int	  _UKLEN;
		bool  _useHash;
		Jstr     _pathName;

};

#endif
