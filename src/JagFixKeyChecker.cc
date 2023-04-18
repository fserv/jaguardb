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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <JagFixKeyChecker.h>
#include <JagUtil.h>
#include <JagMD5lib.h>
#include <JagSingleBuffReader.h>
#include <JagFileMgr.h>

JagFixKeyChecker::JagFixKeyChecker( const Jstr &pathName, int klen, int vlen )
: JagFamilyKeyChecker( pathName, klen, vlen )
{
	if ( _useHash ) {
		_keyCheckArr = new JagFixHashArray( _UKLEN, JAG_KEYCHECKER_VLEN );
	} else {
		_keyCheckArr = new JagFixHashArray( _UKLEN, JAG_KEYCHECKER_VLEN );
	}
}

void JagFixKeyChecker::destroy()
{
	if ( _keyCheckArr ) {
		delete _keyCheckArr;
		_keyCheckArr = NULL;
		jagmalloc_trim( 0 );
	}
}

bool JagFixKeyChecker::addKeyValue( const char *kv )
{
	return addKeyValueNoLock( kv );
}

bool JagFixKeyChecker::addKeyValueNoLock( const char *kv )
{
	char ukey[_UKLEN+1];  
	char uv[_UKLEN+JAG_KEYCHECKER_VLEN];

	getUniqueKey( kv, ukey );  

	memcpy( uv, ukey, _UKLEN );
	memcpy( uv+_UKLEN, kv+_KLEN, JAG_KEYCHECKER_VLEN );

    //dn("s633702 _keyCheckArr->insert uv=[%s]", uv );
	bool rc =  _keyCheckArr->insert( uv );
	return rc; 
}

bool JagFixKeyChecker::addKeyValueInit( const char *kv )
{
	return _keyCheckArr->insert( kv );
}

bool JagFixKeyChecker::getValue( const char *key, char *value )
{
	char ukey[_UKLEN+1];
	getUniqueKey( key, ukey );
	bool rc =  _keyCheckArr->get( ukey, value );
	if ( ! rc ) return false;
	return true;
}

bool JagFixKeyChecker::removeKey( const char *key )
{
	char ukey[_UKLEN+1];
	getUniqueKey( key, ukey );
	return _keyCheckArr->remove( ukey );
}

bool JagFixKeyChecker::exist( const char *key ) const
{
	char ukey[_UKLEN+1]; jagint idx;
	getUniqueKey( key, ukey );
	bool rc = _keyCheckArr->exist( ukey, &idx );
	return rc;
}

void JagFixKeyChecker::removeAllKey()
{
	_keyCheckArr->removeAll();
}

int JagFixKeyChecker::buildInitKeyCheckerFromSigFile()
{
	jd(JAG_LOG_HIGH, "fixkcheck buildInitKeyCheckerFromSigFile ...\n" );
    dn("s333008 buildInitKeyCheckerFromSigFile _pathName=[%s]", _pathName.s() );

	int klen = _UKLEN;
	int vlen = JAG_KEYCHECKER_VLEN;
	char buf[klen+vlen+1];
	memset( buf, 0, klen+vlen+1 );

	Jstr hdbfpath = _pathName + ".hdb";
	Jstr keyCheckerPath = _pathName + ".sig";
    dn("s2022983 hdbfpath=[%s]  keyCheckerPath=[%s]", hdbfpath.s(), keyCheckerPath.s() );

	jagint hdbsize = JagFileMgr::fileSize( hdbfpath );

	if ( hdbsize > 0 && JagFileMgr::fileSize( keyCheckerPath ) < 1 ) {
		int fd = jagopen ( hdbfpath.c_str(), O_RDONLY|JAG_NOATIME );
		if ( fd < 0 ) {
            dn("s01228283 JagFixKeyChecker buildInitKeyCheckerFromSigFile fd=%d < 0 return 0", fd);
			return 0;
		}

		jagint rlimit = getBuffReaderWriterMemorySize( (hdbsize-1)/1024/1024 );
		JagSingleBuffReader nav( fd, hdbsize/(klen+vlen), klen, vlen, 0, 0, rlimit );
		jagint cnt = 0;
		bool rc;
		while( nav.getNext( buf ) ) {
			rc = addKeyValueInit( buf );
			memset( buf, 0, klen+vlen+1 );
			++cnt;
		}
		jagclose( fd );
		jagunlink( hdbfpath.c_str() );
		return 1;
	}

	struct stat sbuf;
	stat(keyCheckerPath.c_str(), &sbuf);
	if ( sbuf.st_size < 1 ) {
		jagunlink( keyCheckerPath.c_str() );
        dn("s50038 file [%s] not found return 0", keyCheckerPath.c_str() ); 
		return 0;
	}

    dn("s5028338 jaopen(%s) ...", keyCheckerPath.c_str() );
	int fd = jagopen((char *)keyCheckerPath.c_str(), O_RDONLY|JAG_NOATIME );
	if ( fd < 0 ) {
        dn("s3038117 jagopen error [%s] fd=%d < 0 return 0", keyCheckerPath.c_str(), fd);
		return 0;
	}
    dn("s5028338 jaopen(%s) OK fd=%d ", keyCheckerPath.c_str() );

    // hdrbyte
    //jagint rdoffset = 0;
    jagint rdoffset = 1;
	raysaferead( fd, buf, 1 );
	if ( buf[0] != '0' ) {
		jagclose( fd );
		jagunlink( keyCheckerPath.c_str() );
        dn("s50038 return 0 here buf[0] != '0'");
		return 0;
	}

	jagint rlimit = getBuffReaderWriterMemorySize( (sbuf.st_size-1)/1024/1024 );

    dn("s70034 using JagSingleBuffReader read fd=%d ...", fd );
	//JagSingleBuffReader br( fd, (sbuf.st_size-1)/(klen+vlen), klen, vlen, 0, 1, rlimit );
	JagSingleBuffReader br( fd, (sbuf.st_size-1)/(klen+vlen), klen, vlen, 0, rdoffset, rlimit );
	jagint cnt = 0; 

	jd(JAG_LOG_LOW, "begin reading sig file ...\n" );
	JagFixString vstr;
	memset( buf, 0, klen+vlen+1 );
	bool rc;

	while ( br.getNext( buf ) ) {
		++cnt;
		rc = addKeyValueInit( buf );
		memset( buf, 0, klen+vlen+1 );
	}

	jd(JAG_LOG_LOW, "done reading sig file %ld records rlimit=%ld unlink sigfile\n", cnt, rlimit );
	jagclose( fd );
	jagunlink( keyCheckerPath.c_str() );
	jagmalloc_trim(0);
	return 1;
}
