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
#ifndef _jag_diskkeychecker_h_
#define _jag_diskkeychecker_h_
#include <stdio.h>
#include <abax.h>
#include <JagDef.h>
#include <JagFamilyKeyChecker.h>
#include <JagLocalDiskHash.h>

class JagDiskKeyChecker  : public JagFamilyKeyChecker
{
    public: 
        JagDiskKeyChecker( const Jstr &fpath, int klen, int vlen );
        virtual ~JagDiskKeyChecker() { destroy(); }
        virtual bool addKeyValue( const char *kv );
        virtual bool addKeyValueNoLock( const char *kv );
        virtual bool getValue( const char *key, char *val );
        virtual bool removeKey( const char *key );
        virtual bool exist( const char *key ) const;
		virtual void removeAllKey( );
        virtual jagint size() const  { return _keyCheckArr->elements(); }
        virtual jagint arrayLength() const  { return 0; }
		virtual const char *array() const { return NULL; }
		virtual bool addKeyValueInit(const char*) { return true; }
		virtual int buildInitKeyCheckerFromSigFile();

    protected:
		JagLocalDiskHash  *_keyCheckArr;
		virtual void  destroy();
		//void  getUniqueKey( const char *key, char *ukey );
		//int   KLEN;
		// JagReadWriteLock    *_lock;
		int readSigToHDB( const Jstr &sigfpath, const Jstr &hdbfpath );
		bool _addSigKeyValue( const char *kv );
		Jstr  _fpath;

};

#endif
