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
#ifndef _jag_fixkeychecker_h_
#define _jag_fixkeychecker_h_
#include <stdio.h>
#include <abax.h>
#include <JagDef.h>
#include <JagFixHashArray.h>
#include <JagFamilyKeyChecker.h>

class JagFixKeyChecker  : public JagFamilyKeyChecker
{

    public: 
        JagFixKeyChecker( const Jstr &pathName, int klen, int vlen );
        virtual ~JagFixKeyChecker() { destroy(); }
        virtual bool addKeyValue( const char *kv );
        virtual bool addKeyValueNoLock( const char *kv );
        virtual bool addKeyValueInit( const char *kv );
        virtual bool getValue( const char *key, char *val );
        virtual bool removeKey( const char *key );
        virtual bool exist( const char *key ) const;
		virtual void removeAllKey( );
        virtual jagint size() const  { return _keyCheckArr->elements(); }
        virtual jagint arrayLength() const  { return _keyCheckArr->arrayLength(); }
		virtual const char *array() const { return _keyCheckArr->array(); }

		virtual int buildInitKeyCheckerFromSigFile();


    protected:
		JagFixHashArray  *_keyCheckArr;
		virtual void  destroy();
		//void  getUniqueKey( const char *key, char *ukey );
		//int   KLEN;
		// JagReadWriteLock    *_lock;

};

#endif
