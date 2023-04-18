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
#include <JagBuffReader.h>

JagBuffReader::JagBuffReader( JagDiskArrayBase *darr, jagint readlen, jagint keylen, jagint vallen, 
							  jagint start, jagint headoffset, jagint bufferSize ) 
	: _darr( darr )
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
	_start = start;
	_readlen = readlen;

    dn("s4500012 JagBuffReader ctor  _headoffset=%ld _start=%ld _readlen=%ld", _headoffset, _start, _readlen );

	_elements = getNumBlocks( KEYVALLEN, bufferSize )*JAG_BLOCK_SIZE;
	if ( KEYVALLEN > 100000 ) { _elements = 2; } 
	
	AbaxString dolock = "NO";
	if ( darr->_servobj && darr->_servobj->_cfg ) {
		dolock = darr->_servobj->_cfg->getValue("BUFF_READER_LOCK", "NO");
	}
	if ( dolock != "NO" ) { _dolock = 1; }
	
	if ( _start < 0 ) { 
		_start = 0;
		_readAll = 1;
		if ( _readlen < 0 ) {
			struct stat sbuf;
			if ( stat(darr->_filePath.c_str(), &sbuf) != 0 || sbuf.st_size/KEYVALLEN == 0 ) {
				d("s222029 fatal error file=[%s] not found\n", darr->_filePath.c_str() );
				abort();
			} 
			_readlen = sbuf.st_size/KEYVALLEN;
		}
	}
	
	_superbuf = NULL;
	_superbuf = (char*)jagmalloc( _elements*KEYVALLEN );
	while ( !_superbuf ) {
		_elements = _elements/2;
		_superbuf = (char*)jagmalloc( _elements*KEYVALLEN );
		if ( _superbuf ) {
			jd(JAG_LOG_LOW, "JagBuffReader malloc smaller memory %ld _elements=%ld _fpath=[%s]\n", 
					_elements*KEYVALLEN, _elements, _darr->getFilePath().c_str() );
		}
	}
	memset( _superbuf, 0, KEYVALLEN );
	
	_n = 0; // for debug
}

JagBuffReader::~JagBuffReader()
{
	if ( _superbuf ) {
		free( _superbuf );
		_superbuf = NULL;
		jagmalloc_trim(0);
	}
}

jagint JagBuffReader::getNumBlocks( jagint kvlen, jagint bufferSize )
{
	jagint num = 4096;
	jagint bytes = num*kvlen*JAG_BLOCK_SIZE;
	jagint maxmb = 4;

	if ( bufferSize > 0 ) {
		maxmb = bufferSize;
		bytes = maxmb * 1024 * 1024;
		num = bytes/kvlen/JAG_BLOCK_SIZE;
	}

	if ( bytes > maxmb * 1024*1024 ) {
		num = maxmb*1024*1024/kvlen/JAG_BLOCK_SIZE;
	}

	return num;
}

bool JagBuffReader::getNext( char *buf )
{
	jagint pos;
	return getNext( buf, KEYVALLEN, pos );
}

bool JagBuffReader::getNext( char *buf, jagint &i )
{
	return getNext( buf, KEYVALLEN, i );
}

bool JagBuffReader::getNext( char *buf, jagint len, jagint &i )
{
	if ( !_darr ) {
		//d("e8394 error read buffer darr empty\n");
		return false;
	}

	if ( len < KEYVALLEN ) {
		//d("s80394 error JagBuffReader::getNext passedin len=%d is less than KEYVALLEN=%d\n", len, KEYVALLEN );
		return false;
	}

	if ( _lastSuperBlock*_elements + _relpos >= _readlen ) {
		//d("s44190 reach to the end\n");
		//d("s44190 _lastSuperBlock=%d _elements=%d _relpos=%d  _readlen=%d false\n", _lastSuperBlock, _elements, _relpos, _readlen);
		return false;
	}

	//d("s44190 _lastSuperBlock=%d _elements=%d _relpos=%d  _readlen=%d ok\n", _lastSuperBlock, _elements, _relpos, _readlen);
	
	jagint rc;
    if ( -1 == _lastSuperBlock ) { 
		if ( _readlen <= _elements ) { 
			_curBlockElements = _readlen;
		} else {
			_curBlockElements = _elements;			
		}

		rc = jdfpread( _darr->_jdfs, _superbuf, _curBlockElements*KEYVALLEN, _start*KEYVALLEN+_headoffset );
        dn("s722992 jdfpread rc=%lld", rc );
		if ( rc <= 0 ) { 
			return false;
		}

        _lastSuperBlock = 0;
		_relpos = 0;
    }

	rc = findNonblankElement( buf, i );
	if ( !rc ) { // no more data
		//d("s222183 no more data false _n=%d _readlen=%d\n", _n, _readlen );
       	return false;
	}

	++ _n;
    return true;
}

bool JagBuffReader::findNonblankElement( char *buf, jagint &i )
{
	jagint rc;
	while ( 1 ) {
		if ( _relpos < _curBlockElements ) {
	 		if ( _superbuf[_relpos*KEYVALLEN] == '\0' ) {
				++_relpos;
				continue;
			} else {
				memcpy(buf, _superbuf+_relpos*KEYVALLEN, KEYVALLEN);
				i = _lastSuperBlock*_elements+_relpos;
				++_relpos;
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

			rc = jdfpread( _darr->_jdfs, _superbuf, _curBlockElements*KEYVALLEN, (_start+_lastSuperBlock*_elements)*KEYVALLEN+_headoffset );
            dn("s19928 jdfpread rc=%ld", rc );
			if ( rc <= 0 ) {
				// no valid bytes read from file
				return false;
			}
		}
	}

    return false;
}

bool JagBuffReader::setRestartPos()
{
	if ( _setRestartPos ) return false;
	_stlastSuperBlock = _lastSuperBlock;
	_strelpos = _relpos-1; 
	_setRestartPos = 1;
	return true;
}

bool JagBuffReader::moveToRestartPos()
{
	if ( !_setRestartPos ) return false;
	jagint rc;
	if ( _stlastSuperBlock >= _lastSuperBlock ) {
		_relpos = _strelpos;
	} else {
		rc = _readlen - _stlastSuperBlock*_elements;
		if ( rc <= 0 ) {
			_setRestartPos = 0;
			return false;
		} else if ( rc < _elements ) {
			_curBlockElements = rc;
		} else {
			_curBlockElements = _elements;
		}

		rc = jdfpread( _darr->_jdfs, _superbuf, _curBlockElements*KEYVALLEN, (_start+_stlastSuperBlock*_elements)*KEYVALLEN+_headoffset );
        dn("s078883 in moveToRestartPos jdfpread rc=%ld", rc );

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

void JagBuffReader::setClearRestartPosFlag() { 
	_setRestartPos = 0; 
}
