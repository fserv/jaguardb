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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <JagFSMgr.h>
#include <JagFileMgr.h>
#include <JagDef.h>
#include <JagUtil.h>
#include <JagCompFile.h>

JagFSMgr::JagFSMgr()
{
	_map = new JagHashMap<AbaxString,AbaxBuffer>( true, 100 );
}

JagFSMgr::~JagFSMgr()
{
	// for each item, close the FD in _map
	JagCompFile *compf;
	for ( int i =0; i < _map->size(); ++i ) {
		if (  _map->isNull(i) ) continue;
		compf = ( JagCompFile*) _map->valueAt(i).value();
		delete compf;
	}

	if ( _map ) delete _map;
	_map = NULL;
}

bool JagFSMgr::exist( const AbaxString &fpath )
{
	struct stat s;
	if ( 0 == stat( fpath.c_str(), &s ) ) {
		return true;
	} else {
		return false;
	}
}

jagint JagFSMgr::getStripeSize( const AbaxString &fpath, size_t kvlen )
{
	dn("s42839 getStripeSize fpath=[%s] kvlen=%d", fpath.c_str(), kvlen );
	JagCompFile *compf = getCompf( fpath );
	if ( compf ) {
		if ( compf->size() > 0 ) {
			return compf->size()/kvlen;
		}
	}

	return -1;
}

jagint JagFSMgr::getFileSize( const AbaxString &fpath, size_t kvlen )
{
    dn("s00238813 getFileSize");
	JagCompFile *compf = getCompf( fpath );
	if ( compf ) {
		if ( compf->size() > 0 ) {
			return compf->size()/kvlen;
		}
	}
	return 0;
}
	
// check if file is open. if no, open and insert to hashmap
JagCompFile *JagFSMgr::openf( JagDiskArrayFamily *fam, const AbaxString &fpath, jagint klen, jagint vlen, bool force )
{
    dn("s00238813 JagFSMgr::openf %s force=%d", fpath.s(), force );
	JagCompFile *compf = getCompf( fpath );
	if ( compf ) return compf;

	if ( force || exist( fpath.c_str() ) ) {
		d("s10029 JagFSMgr::open() new JagCompFile klen=%d vlen=%d\n", klen, vlen );
		compf = new JagCompFile( fam, fpath.c_str(), klen, vlen );
	} else {
		return NULL;
	}

	_map->addKeyValue( fpath, AbaxBuffer( compf) );
	return compf;
}

// check if file is open. if no, open and insert to hashmap
int JagFSMgr::openfd( const AbaxString &fpath, bool force )
{
	int fd = getFileDesc( fpath );
	if ( fd >= 0 ) return fd;

	if ( force || exist( fpath.c_str() ) ) {
		//d("s10029 JagFSMgr::open() new JagCompFile klen=%d vlen=%d\n", klen, vlen );
		fd = ::open( fpath.c_str(), O_CREAT|O_RDWR|JAG_NOATIME, S_IRWXU);
	} else {
		return -1;
	}

	jaguint addr = (jaguint)fd;
	_map->addKeyValue( fpath, AbaxBuffer( (void*)addr ) );
	return fd;
}

// check if file is open. if yes, close and remove from hashmap
int JagFSMgr::closef( const AbaxString &fpath )
{
    dn("s09322 closef");
	JagCompFile *compf = getCompf( fpath );
	if ( ! compf ) {
		return -1;
	}

	delete compf; 
	_map->removeKey( fpath );
	return 1;
}

// check if file is open. if yes, close and remove from hashmap
int JagFSMgr::closefd( const AbaxString &fpath )
{
	int fd = getFileDesc( fpath );
	if ( fd < 0 ) {
		return -1;
	}

	_map->removeKey( fpath );
	::close( fd );
	return 1;
}

JagCompFile* JagFSMgr::getCompf( const AbaxString &fpath )
{
	AbaxBuffer ptr;
	if ( ! _map->getValue( fpath, ptr ) ) {
		return NULL;
	}

	return (JagCompFile*)ptr.value();
}

int JagFSMgr::getFileDesc( const AbaxString &fpath )
{
	AbaxBuffer ptr;
	if ( ! _map->getValue( fpath, ptr ) ) {
		return -1;
	}

	void *pv = ptr.value();
	jaguint ui = (jaguint)pv;
	return (int)ui; 
}

// rename a file
int JagFSMgr::rename( const AbaxString &fpath, const AbaxString &newfpath )
{
	JagCompFile *compf = getCompf( fpath );
	if ( ! compf ) return -1;

	_map->removeKey( fpath );
	_map->addKeyValue( newfpath, AbaxBuffer( compf ) );
	return 1;
}

// remove a file
int JagFSMgr::remove( const AbaxString &fpath )
{
	if ( ! exist( fpath ) ) {
		return -1;
	}

	/**
	close( fpath );
	jagunlink( fpath.c_str() );
	return 1;
	**/
	JagCompFile *compf = getCompf( fpath );
	if ( ! compf ) return -1;

	compf->removeFile();
	delete compf;
	_map->removeKey( fpath );
	return 0;
}

jagint JagFSMgr::pread(  const JagCompFile *compf, void *buf, size_t len, jagint offset)
{
    dn("s980031 JagFSMgr::pread offset=%ld len=%ld", offset, len);
	jagint rc =  compf->pread( (char*)buf, len, offset );
    dn("s008111 JagFSMgr::pread() compf->pread rc=%ld", rc );
	return rc;
}

jagint JagFSMgr::pwrite( JagCompFile *compf, const void *buf, size_t count, jagint offset)
{
	return compf->pwrite( (char*)buf, count, offset );
}

jagint JagFSMgr::pread( int fd,  void *buf, size_t count, jagint offset)
{
	return jagpread( fd, (char*)buf, count, offset );
}

jagint JagFSMgr::pwrite( int fd, const void *buf, size_t count, jagint offset)
{
	return jagpwrite( fd, (const char*)buf, count, offset );
}

