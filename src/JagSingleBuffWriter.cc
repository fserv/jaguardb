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

#include <JagSingleBuffWriter.h>
#include <JagUtil.h>
#include <JagCfg.h>
#include <JagDef.h>
#include <JagCompFile.h>

JagSingleBuffWriter::JagSingleBuffWriter( JagCompFile *compf, int kvlen, jagint bufferSize )
{
	KVLEN = kvlen;
	_compf = compf;
	_superbuf = NULL;
	if ( ! _compf ) { 
		d("s502348 JagSingleBuffWriter ctor1 return\n");
		return; 
	}
	_fd = 1;

	init( kvlen, bufferSize );
}

JagSingleBuffWriter::JagSingleBuffWriter( int fd, int kvlen, jagint bufferSize )
{
    _fd = fd;
    _superbuf = NULL;
    if ( _fd < 0 ) { 
		d("s502358 JagSingleBuffWriter ctor2 return\n");
		return; 
	}
	_compf = nullptr;

	init( kvlen, bufferSize );
}

void JagSingleBuffWriter::init( int kvlen, jagint bufferSize )
{
    KVLEN = kvlen;
    if ( bufferSize == -1 ) {
        //bufferSize = 128;
        bufferSize = 32;
    }

	d("s81029 init kvlen=%d bufferSize=%d\n", kvlen, bufferSize );

    SUPERBLOCK = bufferSize*1024*1024/KVLEN/JagCfg::_BLOCK*JagCfg::_BLOCK;
    SUPERBLOCKLEN = SUPERBLOCK * KVLEN;
    _superbuf = (char*) jagmalloc( SUPERBLOCKLEN );
    _lastSuperBlock = -1;
    _relpos = -1;
	d("s12876 _superbuf SUPERBLOCKLEN=%d\n", SUPERBLOCKLEN );

    memset( _superbuf, 0, SUPERBLOCKLEN );
}


JagSingleBuffWriter::~JagSingleBuffWriter()
{
	if ( _superbuf ) {
		free( _superbuf );
		_superbuf = NULL;
		jagmalloc_trim( 0 );
		//jd(JAG_LOG_HIGH, "s2829 sigbufwtr SUPERBLOCKLEN=%ld dtor\n", SUPERBLOCKLEN );
	}
}

/***
void JagSingleBuffWriter::resetKVLEN( int newkvlen )
{
	KVLEN = newkvlen;
	SUPERBLOCKLEN = SUPERBLOCK * KVLEN;
	if ( _superbuf ) {
		free( _superbuf );
	}
	_superbuf = (char*) malloc( SUPERBLOCKLEN );
    _lastSuperBlock = -1;
    _relpos = -1;
	
	memset( _superbuf, 0, SUPERBLOCKLEN );
}
***/

// pos is not absolute position, it is index, increamented one by one. position is pos*KVLEN
void JagSingleBuffWriter::writeit( jagint pos, const char *keyvalbuf, jagint KVLEN )
{
	//d("s33222 JagSingleBuffWriter::writeit pos=%d keyvalbuf=[%s] KVLEN=%d ...\n", pos, keyvalbuf, KVLEN);
	_relpos = pos % SUPERBLOCK;  // relative positon inside a superblock (SUPERBLOCKLEN = SUPERBLOCK * KVLEN)
	jagint rc; 
	int newBlock = pos / SUPERBLOCK;
	if ( -1 == _lastSuperBlock ) {
		memcpy( _superbuf+_relpos*KVLEN, keyvalbuf, KVLEN );
		//d("s239311 memcpy[%s] to _superbuf at location=%d _relpos*KVLEN=%d\n", keyvalbuf, _relpos, _relpos*KVLEN );
		_lastSuperBlock = newBlock;
		return;
	}

	if ( newBlock == _lastSuperBlock ) {
		memcpy( _superbuf+_relpos*KVLEN, keyvalbuf, KVLEN );
		//d("s239313 memcpy[%s] to _superbuf at location=%d _relpos*KVLEN=%d\n", keyvalbuf, _relpos, _relpos*KVLEN );
		return;
	} 

	// moved to new block, flush old block
	// rc = raysafepwrite( _compf, _superbuf, SUPERBLOCKLEN, _lastSuperBlock*SUPERBLOCKLEN );
	if ( _compf ) {
		//d("s440293 _compf->pwrite ...\n");
		rc = _compf->pwrite( _superbuf, SUPERBLOCKLEN, _lastSuperBlock*SUPERBLOCKLEN );
	} else {
		//d("s440294 raysafepwrite _fd=%d ...\n", _fd );
		rc = raysafepwrite( _fd, _superbuf, SUPERBLOCKLEN, _lastSuperBlock*SUPERBLOCKLEN );
	}

	memset( _superbuf, 0, SUPERBLOCKLEN );
	memcpy( _superbuf+_relpos*KVLEN, keyvalbuf, KVLEN );
	_lastSuperBlock = newBlock;
	return;
}

void JagSingleBuffWriter::flushBuffer()
{
	if ( _lastSuperBlock == -1 ) { return; }
	//jagint rc = raysafepwrite( _compf, _superbuf, (_relpos+1)*KVLEN, _lastSuperBlock*SUPERBLOCKLEN );
	if ( _compf ) {
		_compf->pwrite( _superbuf, (_relpos+1)*KVLEN, _lastSuperBlock*SUPERBLOCKLEN );
	} else {
		//d("s322287 flushBuffer _relpos+1=%d _lastSuperBlock=%d \n", _relpos+1, _lastSuperBlock );
		raysafepwrite( _fd, _superbuf, (_relpos+1)*KVLEN, _lastSuperBlock*SUPERBLOCKLEN );
	}
	_lastSuperBlock = -1;
	_relpos = -1;
	return;
}
