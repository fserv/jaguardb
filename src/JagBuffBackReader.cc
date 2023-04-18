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

#include <fcntl.h>
#include <sys/stat.h>

#include <JDFS.h>
#include <JagCfg.h>
#include <JagUtil.h>
#include <JagDBPair.h>
#include <JagDiskArrayBase.h>
#include <JagBuffBackReader.h>

JagBuffBackReader::JagBuffBackReader( JagDiskArrayBase *darr, jagint readlen, jagint keylen, jagint vallen, jagint end, 
	jagint headoffset, jagint bufferSize )  : _darr( darr )
{
	KEYLEN = keylen; 
	VALLEN = vallen;
	KEYVALLEN = keylen + vallen;
	_dolock = 0;
	_readAll = 0;
	_setRestartPos = 0;
	_lastSuperBlock = -1;
	_relpos = 0;
	_stlastSuperBlock = -1;
	_strelpos = 0;
	_curBlockElements = 0;
	_headoffset = headoffset;
	_end = end;
	_readlen = readlen;
	_elements = getNumBlocks( KEYVALLEN, bufferSize )*JAG_BLOCK_SIZE;
	if ( KEYVALLEN > 100000 ) { _elements = 2; } 
	
	AbaxString dolock = "NO";
	if ( darr->_servobj && darr->_servobj->_cfg ) {
		dolock = darr->_servobj->_cfg->getValue("BUFF_READER_LOCK", "NO");
	}
	if ( dolock != "NO" ) { _dolock = 1; }
	
	if ( _end < 0 ) { 
		_end = 0;
		_readAll = 1;
		if ( _readlen < 0 ) {
			struct stat sbuf;
			if ( stat(darr->_filePath.c_str(), &sbuf) != 0 || sbuf.st_size/KEYVALLEN == 0 ) return;
			_readlen = sbuf.st_size/KEYVALLEN;
		}
		_end = _readlen;
	}
	
	_superbuf = NULL;
	jd(JAG_LOG_HIGH, "s2280 bufbckrdr _elements=%ld mem=%ld\n", _elements, _elements * (jagint)KEYVALLEN );
	_superbuf = (char*)jagmalloc( _elements*KEYVALLEN );
	while ( !_superbuf ) {
		_elements = _elements/2;
		_superbuf = (char*)jagmalloc( _elements*KEYVALLEN );
		if ( _superbuf ) {
			jd(JAG_LOG_LOW, "JagBuffBackReader malloc smaller memory %ld _elements=%ld _fpath=[%s]\n", 
					_elements*KEYVALLEN, _elements, _darr->getFilePath().c_str() );
		}
	}
	memset( _superbuf, 0, KEYVALLEN );
}

JagBuffBackReader::~JagBuffBackReader()
{
	if ( _superbuf ) {
		free( _superbuf );
		_superbuf = NULL;
		jagmalloc_trim(0);
	}
}

jagint JagBuffBackReader::getNumBlocks( jagint kvlen, jagint bufferSize )
{
	jagint num = 4096;
	jagint bytes = num*kvlen*JAG_BLOCK_SIZE;
	jagint maxmb = 4;

	if ( bufferSize > 0 ) {
		maxmb = bufferSize;
		bytes = maxmb * 1024 * 1024;
		num = bytes/kvlen/JagCfg::_BLOCK;
	}

	if ( bytes > maxmb * 1024*1024 ) {
		num = maxmb*1024*1024/kvlen/JagCfg::_BLOCK;
	}

	return num;
}

bool JagBuffBackReader::getNext( char *buf )
{
	jagint pos;
	return getNext( buf, KEYVALLEN, pos );
}

bool JagBuffBackReader::getNext( char *buf, jagint &i )
{
	return getNext( buf, KEYVALLEN, i );
}

bool JagBuffBackReader::getNext( char *buf, jagint len, jagint &i )
{
	if ( !_darr ) {
		return false;
	}
	if ( len < KEYVALLEN ) {
		return false;
	}
	if ( _lastSuperBlock*_elements+_curBlockElements-1-_relpos >= _readlen ) {
		return false;
	}
	
	jagint rc;
    if ( -1 == _lastSuperBlock ) { 
		if ( _readlen <= _elements ) {  
			_curBlockElements = _readlen;
		} else {
			_curBlockElements = _elements;			
		}
		rc = jdfpread( _darr->_jdfs, _superbuf, _curBlockElements*KEYVALLEN, (_end-_curBlockElements)*KEYVALLEN+_headoffset );
		if ( rc <= 0 ) { 
			return false;
		}
        _lastSuperBlock = 0;
		_relpos = _curBlockElements-1;
    }

	rc = findNonblankElement( buf, i );
	if ( !rc ) { 
       	return false;
	}

    return true;
}

bool JagBuffBackReader::findNonblankElement( char *buf, jagint &i )
{
	jagint rc;
	while ( 1 ) {
		if ( _relpos >= 0 ) {
	 		if ( _superbuf[_relpos*KEYVALLEN] == '\0' ) {
				--_relpos;
				continue;
			} else {
				memcpy(buf, _superbuf+_relpos*KEYVALLEN, KEYVALLEN);
				i = _lastSuperBlock*_elements+_curBlockElements-1-_relpos;
				--_relpos;
				return true;
			}
		} else if ( _curBlockElements < _elements ) {
			return false;
		} else {
			_relpos = 0;
			++ _lastSuperBlock;

			rc = _readlen-_lastSuperBlock*_elements;
			if ( rc <= 0 ) {
				return false;
			} else if ( rc < _elements ) {
				_curBlockElements = rc;
			} else {
				_curBlockElements = _elements;
			}
			rc = jdfpread( _darr->_jdfs, _superbuf, _curBlockElements*KEYVALLEN, (_end-_lastSuperBlock*_elements-_curBlockElements)*KEYVALLEN+_headoffset );
			if ( rc <= 0 ) {
				return false;
			}
			_relpos = _curBlockElements-1;
		}
	}

    return false;
}

bool JagBuffBackReader::setRestartPos()
{
	if ( _setRestartPos ) return false;
	_stlastSuperBlock = _lastSuperBlock;
	_strelpos = _relpos+1; 
	_setRestartPos = 1;
	return true;
}

bool JagBuffBackReader::moveToRestartPos()
{
	if ( !_setRestartPos ) return false;
	jagint rc;
	if ( _stlastSuperBlock >= _lastSuperBlock ) {
		_relpos = _strelpos;
	} else {
		rc = _readlen-_stlastSuperBlock*_elements;
		if ( rc <= 0 ) {
			_setRestartPos = 0;
			return false;
		} else if ( rc < _elements ) {
			_curBlockElements = rc;
		} else {
			_curBlockElements = _elements;
		}
		rc = jdfpread( _darr->_jdfs, _superbuf, _curBlockElements*KEYVALLEN, (_end-_stlastSuperBlock*_elements-_curBlockElements)*KEYVALLEN+_headoffset );
		if ( rc <= 0 ) {
			_setRestartPos = 0;
			return false;
		}
		_relpos = _strelpos;
		_lastSuperBlock = _stlastSuperBlock;
		_setRestartPos = 0;
		return true;
	}
	return false;
}

void JagBuffBackReader::setClearRestartPosFlag() { 
	_setRestartPos = 0; 
}
