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

#include "JagIndexSchema.h"
#include "JagDBServer.h"

JagIndexSchema::JagIndexSchema( JagDBServer *serv, int replicType )
  :JagSchema( )
{
	this->init( serv, "INDEX", replicType );
}

JagIndexSchema::~JagIndexSchema()
{
}

int JagIndexSchema::getIndexNames( const Jstr &dbname, const Jstr &tabname, 
									JagVector<Jstr> &vec )
{
	JAG_BLURT JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	bool rc;
	jagint getpos;

	char *buf = (char*) jagmalloc ( KVLEN+1 );
	memset( buf, '\0', KVLEN+1 );

	char *keybuf = (char*) jagmalloc ( KEYLEN+1 );

	jagint length = _schema->getLength();
	JagSingleBuffReader nti( _schema->getFD(), length, KEYLEN, VALLEN, 0, 0, 1 );
	JagColumn onecolrec;
	Jstr  ks;

	int cnt = 0;
	while ( true ) {
		rc = nti.getNext(buf, KVLEN, getpos);
		if ( !rc ) { break; }
		memset( keybuf, '\0', KEYLEN+1 );
		memcpy( keybuf, buf, KEYLEN );
		ks = buf;
        dn("s02091 getIndexNames ks=[%s]", ks.s() );
		JagStrSplit split( ks, '.' );
		if ( split.length() < 3 ) { continue; }

		if ( dbname == split[0] && tabname == split[1] ) {
			vec.append( split[2] );
            dn("s020281 use and append [%s]", split[2].s() );
			++ cnt;
		}
	}

	if ( buf ) free ( buf );
	if ( keybuf ) free ( keybuf );

	return cnt;
}

// use memory to read instead of disk, _recordMap, to get all index names under one table
// returns number of items found
int JagIndexSchema::getIndexNamesFromMem( const Jstr &dbname, const Jstr &tabname, 
											JagVector<Jstr> &vec )
{
	JAG_BLURT JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );

	Jstr dbtab = dbname + "." + tabname + ".";
	jagint hdrlen = dbtab.size();
    jagint len = _recordMap->arrayLength();
    dn("s020281 getIndexNamesFromMem _recordMap.arrlen=%ld", len);

    const AbaxPair<AbaxString, AbaxString> *arr = _recordMap->array();
    int  cnt = 0;
    for ( jagint i = 0; i < len; ++i ) {
    	if ( _recordMap->isNull(i) ) continue;
        dn("s02097 getIndexNamesFromMem arr[i=%d]=[%s] dbtab=[%s] hdrlen=%d", i, arr[i].key.s(), dbtab.s(), hdrlen );

		if ( memcmp(arr[i].key.c_str(), dbtab.c_str(), hdrlen) != 0 ) { continue; }

        Jstr s(arr[i].key.c_str() + hdrlen);
        dn("s022636 use and append [%s]", s.s() );
		vec.append( s );
        ++cnt;
    }

	return cnt;
}

// check if the table exists already from index schema file
bool JagIndexSchema::tableExist( const Jstr &dbname, const JagParseParam *parseParam )
{
	Jstr tab = parseParam->objectVec[0].tableName;
	return tableExist( dbname, tab );
}

bool JagIndexSchema::tableExist( const Jstr &dbname, const Jstr &tab )
{
	JAG_BLURT JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	bool found = false;
	jagint totlen = _schema->getLength();
	char  save;
    jagint getpos;
	int rc;

	char *buf = (char*) jagmalloc ( KVLEN+1 );
	char *keybuflow = (char*) jagmalloc ( KEYLEN+1 );
    memset( buf, '\0', KVLEN+1 );
    memset( keybuflow, 0, KEYLEN+1 );
	memcpy(keybuflow, dbname.c_str(), dbname.size());
	*(keybuflow+dbname.size()) = '.';

	JagSingleBuffReader nti( _schema->getFD(), totlen, KEYLEN, VALLEN, 0, 0, 1 );
	while ( nti.getNext(buf, KVLEN, getpos) ) {
		rc = memcmp(buf, keybuflow, dbname.size()+1);
		if ( rc > 0 ) {
			break;
		}

		save = buf[KEYLEN];
		buf[KEYLEN] = '\0';
		JagStrSplit oneSplit( buf, '.' );
		buf[KEYLEN] = save;
		if ( oneSplit.length() < 3 ) { continue; }

		if ( 0 == rc && oneSplit[1] == tab ) {
			found = true;
			break;
		}
	}

	if ( buf ) free ( buf );
	if ( keybuflow ) free ( keybuflow );

	return found;
}

// check if the index exists already from index schema file
bool JagIndexSchema::indexExist( const Jstr &dbname, const JagParseParam *parseParam )
{
	Jstr idx  = parseParam->objectVec[0].indexName;
	return indexExist( dbname, idx );
}

bool JagIndexSchema::indexExist( const Jstr &dbname, const Jstr &idx )
{
	JAG_BLURT JagReadWriteMutex mutex( _lock, JagReadWriteMutex::READ_LOCK );
	jagint totlen = _schema->getLength();
	bool found = false;
	int rc;
	char  save;
    jagint getpos;

	char *buf = (char*) jagmalloc ( KVLEN+1 );
	char *keybuflow = (char*) jagmalloc ( KEYLEN+1 );
    memset( buf, '\0', KVLEN+1 );
    memset( keybuflow, 0, KEYLEN+1 );
	memcpy(keybuflow, dbname.c_str(), dbname.size());
	*(keybuflow+dbname.size()) = '.';
	
	JagSingleBuffReader nti( _schema->getFD(), totlen, KEYLEN, VALLEN, 0, 0, 1 );
	while ( nti.getNext(buf, KVLEN, getpos) ) {
		rc = memcmp(buf, keybuflow, dbname.size()+1);
		if ( rc > 0 ) {
			// no need to read more
			break;
		}

		save = buf[KEYLEN];
		buf[KEYLEN] = '\0';
		JagStrSplit oneSplit( buf, '.' );
		buf[KEYLEN] = save;
		if ( oneSplit.length() < 3 ) { continue; }

		if ( 0 == rc && oneSplit[2] == idx ) {
			found = true;
			break;
		}
	}
	if ( buf ) free ( buf );
	if ( keybuflow ) free ( keybuflow );

	return found;
}

