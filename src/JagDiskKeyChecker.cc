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

#include <JagDiskKeyChecker.h>
#include <JagUtil.h>
#include <JagMD5lib.h>
#include <JagFileMgr.h>

JagDiskKeyChecker::JagDiskKeyChecker( const Jstr &fpath, int klen, int vlen )
: JagFamilyKeyChecker( fpath, klen, vlen )
{
	_fpath = fpath;

	if ( _useHash ) {
		_keyCheckArr = new JagLocalDiskHash( fpath, _UKLEN, JAG_KEYCHECKER_VLEN );
	} else {
		_keyCheckArr = new JagLocalDiskHash( fpath, _UKLEN, JAG_KEYCHECKER_VLEN );
	}
}

void JagDiskKeyChecker::destroy()
{
	if ( _keyCheckArr ) {
		delete _keyCheckArr;
		_keyCheckArr = NULL;
		jagmalloc_trim( 0 );
	}
}

bool JagDiskKeyChecker::addKeyValue( const char *kv )
{
	return addKeyValueNoLock( kv );
}

bool JagDiskKeyChecker::addKeyValueNoLock( const char *kv )
{
	char ukey[_UKLEN+1];

	getUniqueKey( kv, ukey );  

	JagFixString key( ukey, _UKLEN );
	JagFixString val( kv+_KLEN, JAG_KEYCHECKER_VLEN );

	JagDBPair pair( key, val );
	bool rc = _keyCheckArr->insert( pair );
	return rc; 
}

bool JagDiskKeyChecker::getValue( const char *key, char *value )
{
	char ukey[_UKLEN+1];
	getUniqueKey( key, ukey ); 

	JagFixString pkey( ukey, _UKLEN );
	JagDBPair pair( pkey );
	bool rc =  _keyCheckArr->get( pair );
	if ( ! rc ) return false;
	memcpy( value, pair.value.addr(), JAG_KEYCHECKER_VLEN );
	return true;
}

bool JagDiskKeyChecker::removeKey( const char *key )
{
	char ukey[_UKLEN+1];
	getUniqueKey( key, ukey ); 

	JagFixString pkey( ukey, _UKLEN );
	JagDBPair pair( pkey );
	return _keyCheckArr->remove( pair );
}

bool JagDiskKeyChecker::exist( const char *key ) const
{
	char ukey[_UKLEN+1];
	getUniqueKey( key, ukey ); 

	JagFixString pkey( ukey, _UKLEN );
	JagDBPair pair( pkey );
	bool rc = _keyCheckArr->exist( pair );
	return rc;
}

void JagDiskKeyChecker::removeAllKey()
{
	_keyCheckArr->drop();
}

int JagDiskKeyChecker::buildInitKeyCheckerFromSigFile()
{
	jd(JAG_LOG_LOW, "diskcheck buildInitKeyCheckerFromSigFile ...\n" );

	int rc = 0;
	Jstr sigfpath = _fpath + ".sig";
	Jstr hdbfpath = _fpath + ".hdb";
	jagint sigsize = JagFileMgr::fileSize( sigfpath );
	jagint hdbsize = _keyCheckArr->elements();
	jd(JAG_LOG_LOW, "sigfile=%s\n", sigfpath.c_str() );
	jd(JAG_LOG_LOW, "hdbfile=%s\n", hdbfpath.c_str() );
	jd(JAG_LOG_LOW, "sigsize=%ld hdbsize=%ld\n", sigsize, hdbsize );

	if ( sigsize > 0 && hdbsize < 1 ) {
		jd(JAG_LOG_LOW, "s3629 sig %s [yes]\n", sigfpath.c_str()  );
		jd(JAG_LOG_LOW, "s3629 hdb %s [no], use sigfile\n", hdbfpath.c_str() );
		rc = readSigToHDB( sigfpath, hdbfpath );
		if ( rc < 0 ) {
			jd(JAG_LOG_LOW, "s3623 error reading sigfile\n" );
			return 0;
		}
		jd(JAG_LOG_LOW, "s3423 reading sigfile done\n" );
		return 1;
	}

	if ( sigsize < 1 && hdbsize < 1 ) {
		jd(JAG_LOG_LOW, "s3229 sig %s [no]\n", sigfpath.c_str()  );
		jd(JAG_LOG_LOW, "s3229 hdb %s [no], use sigfile\n", hdbfpath.c_str() );
		return 0;
	}

	jagunlink( sigfpath.c_str() );
	return 1;
}

int JagDiskKeyChecker::readSigToHDB( const Jstr &sigfpath, const Jstr &hdbfpath )
{
	struct stat sbuf;
	stat( sigfpath.c_str(), &sbuf);
	if ( sbuf.st_size < 1 ) {
		jagunlink( sigfpath.c_str() );
		return -70;
	}

	int fd = jagopen((char *)sigfpath.c_str(), O_RDONLY|JAG_NOATIME );
	if ( fd < 0 ) {
		return -100;
	}

	int klen = _UKLEN;
	int vlen = JAG_KEYCHECKER_VLEN;
	char buf[klen+vlen+1];
    memset( buf, 0, klen+vlen+1 );

    // hdrbyte
    //jagint rdoffset = 0;
    jagint rdoffset = 1;
	raysaferead( fd, buf, 1 );
	if ( buf[0] != '0' ) {
		jagclose( fd );
		jagunlink( sigfpath.c_str() );
		return -80;
	}

	jagint rlimit = getBuffReaderWriterMemorySize( (sbuf.st_size-1)/1024/1024 );

	//JagSingleBuffReader br( fd, (sbuf.st_size-1)/(klen+vlen), klen, vlen, 0, 1, rlimit );
	JagSingleBuffReader br( fd, (sbuf.st_size-1)/(klen+vlen), klen, vlen, 0, rdoffset, rlimit );

    memset( buf, 0, klen+vlen+1 );
	jagint cnt = 0;
	bool rc;

	while ( br.getNext( buf ) ) {
		rc = _addSigKeyValue( buf );
		++cnt;
    	memset( buf, 0, klen+vlen+1 );
	}

	jagclose( fd );
	jagunlink( sigfpath.c_str() );

	return 0;
}

bool JagDiskKeyChecker::_addSigKeyValue( const char *kv )
{
	JagDBPair pair( kv, _UKLEN, kv+_UKLEN, JAG_KEYCHECKER_VLEN, true );
	bool rc = _keyCheckArr->insert( pair );
	return rc; 
}
