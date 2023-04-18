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

#include <JagDBServer.h>
#include <JDFS.h>
#include <JagFSMgr.h>
#include <JagUtil.h>
#include <JagDiskArrayFamily.h>

// fpath is full path of a file, without stripe number
// all JDFS methods process localfile only
JDFS::JDFS( JagDBServer *servobj, JagDiskArrayFamily *fam, const Jstr &fpath, int klen, int vlen )
{
	_klen = klen;
	_vlen = vlen;
	_kvlen = klen + vlen;
	_fpath = fpath;
	_servobj = servobj;
	_family = fam;

	if ( _servobj ) {
		_jdfsMgr = _servobj->jdfsMgr;
	} else {
		_jdfsMgr = new JagFSMgr();
	}
	_stripeSize = _jdfsMgr->getStripeSize( _fpath, _kvlen ); // # of kvlens, ie., arrlen
}

JDFS::~JDFS()
{
	close();

	if ( !_servobj ) {
		delete _jdfsMgr;
	}
}

// get local host file descriptor
JagCompFile *JDFS::getCompf( ) 
{
    dn("s99001 getCompf _jdfsMgr->openf _fpath=[%s]", _fpath.s() );
	return _jdfsMgr->openf( _family, _fpath, _klen, _vlen );
}

int JDFS::exist() const
{
	return _jdfsMgr->exist( _fpath );
}

JagCompFile* JDFS::open()
{
    dn("s12297 JDFS::open _jdfsMgr->openf(%s)", _fpath.s() );
	return _jdfsMgr->openf( _family, _fpath, _klen, _vlen, true );
}

int JDFS::close()
{
	return _jdfsMgr->closef( _fpath );
}

int JDFS::rename( const Jstr &newpath )
{
	_jdfsMgr->rename( _fpath, newpath );
	_fpath = newpath;
	close();
	open();
	return 1;
}

int JDFS::remove()
{
	return _jdfsMgr->remove( _fpath );
}

int JDFS::fallocate( jagint offset, jagint len )
{
	int rc = 0;
	JagCompFile *fd = _jdfsMgr->openf( _family, _fpath, _klen, _vlen, true );
	if ( NULL == fd ) rc = -1;
	return rc;
}


jagint JDFS::getArrayLength() const
{
	if ( !exist() ) return 0;
	return _stripeSize;
}

// read data from local
jagint JDFS::pread( char *buf, size_t len, size_t offset ) const
{
	if ( offset < 0 ) offset = 0;
	JagCompFile *compf = _jdfsMgr->openf( _family, _fpath, _klen, _vlen );
	if ( ! compf ) {
		return -1;
	}

	offset = _jdfsMgr->pread( compf, buf, len, offset );
    dn("s2020777 in JDFS::pread() _jdfsMgr->pread offset=%lld", offset );
	return offset;
}

// write data to local
jagint JDFS::pwrite( const char *buf, size_t len, size_t offset )
{
	if ( offset < 0 ) offset = 0;

    dn("s90872 JDFS::pwrite len=%d fpath=%s", len, _fpath.s());

	JagCompFile *compf = _jdfsMgr->openf( _family, _fpath, _klen, _vlen );
	if ( ! compf ) {
		return -1;
	}

	offset = _jdfsMgr->pwrite( compf, buf, len, offset );
	return offset;
}

// use jdfs to read data
ssize_t jdfpread( const JDFS *jdfs, char *buf, jagint len, jagint startpos )
{
	ssize_t s;

	#ifdef DEBUG_LATENCY
		JagClock clock;
		clock.start();
	#endif

	s = jdfs->pread( buf, len, startpos );
    dn("s801227 in jdfpread jdfs->pread s=%lld", s );

	#ifdef DEBUG_LATENCY
		clock.stop();
		printf("s5524 jdfpread(len=%d startpos=%d) took %d microsecs\n", len, startpos, clock.elapsedusec() );
	#endif

	return s;
}

// use jdfs to write data
ssize_t jdfpwrite( JDFS *jdfs, const char *buf, jagint len, jagint startpos )
{
	ssize_t s;

	#ifdef DEBUG_LATENCY
		JagClock clock;
		clock.start();
	#endif

	s = jdfs->pwrite( buf, len, startpos );
	#ifdef DEBUG_LATENCY
		clock.stop();
		printf("s5524 jdfpwrite(len=%d startpos=%d) took %d microsecs\n", len, startpos, clock.elapsedusec() );
	#endif
	return s;
}

