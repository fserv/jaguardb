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

#include <JagBuffWriter.h>
#include <JagUtil.h>
#include <JDFS.h>

JagBuffWriter::JagBuffWriter( JDFS *jdfs, int kvlen, jagint headoffset, jagint bufferSize )
{
	_jdfs = jdfs;
	KVLEN = kvlen;

	if ( bufferSize == -1 ) {
		bufferSize = 32;
	}
	SUPERBLOCK = bufferSize*1024*1024/KVLEN/JagCfg::_BLOCK*JagCfg::_BLOCK;
	SUPERBLOCKLEN = SUPERBLOCK * KVLEN;

	_superbuf = (char*) jagmalloc( SUPERBLOCKLEN );
	_lastBlock = -1;
	_relpos = -1;
	_headoffset = headoffset;

	memset( _superbuf, 0, SUPERBLOCKLEN );
}

JagBuffWriter::~JagBuffWriter()
{
	if ( _superbuf ) {
		free( _superbuf );
		_superbuf = NULL;
		jagmalloc_trim( 0 );
	}
}

void JagBuffWriter::writeit( jagint pos, const char *keyvalbuf, jagint KVLEN )
{
	_relpos = (pos-_headoffset/KVLEN) % SUPERBLOCK;  // elements
	int newBlock = (pos-_headoffset/KVLEN) / SUPERBLOCK;
	if ( -1 == _lastBlock ) {
		memcpy( _superbuf+_relpos*KVLEN, keyvalbuf, KVLEN );
		_lastBlock = newBlock;
		return;
	}

	if ( newBlock == _lastBlock ) {
		memcpy( _superbuf+_relpos*KVLEN, keyvalbuf, KVLEN );
		return;
	} else { 
		raypwrite( _jdfs, _superbuf, SUPERBLOCKLEN, _lastBlock * SUPERBLOCKLEN + _headoffset );
		memset( _superbuf, 0, SUPERBLOCKLEN );
		memcpy( _superbuf+_relpos*KVLEN, keyvalbuf, KVLEN );
		_lastBlock = newBlock;
		return;
	}
}

void JagBuffWriter::flushBuffer()
{
	if ( _lastBlock == -1 ) {
		return;
	}
	raypwrite( _jdfs, _superbuf, (_relpos+1)*KVLEN, _lastBlock*SUPERBLOCKLEN + _headoffset );
	_lastBlock = -1;
	_relpos = -1;
	return;
}

