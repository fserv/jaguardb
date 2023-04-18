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

#include <JagSingleBuffReader.h>
#include <JagCfg.h>
#include <JagUtil.h>
#include <JagCompFile.h>


// readlen is number of KEYVAL records, not bytes
JagSingleBuffReader::JagSingleBuffReader ( JagCompFile *compf, jagint readlen, int keylen, int vallen, 
										   jagint start, jagint headoffset, jagint bufferSize ) 
{
	_intfd = -1;
	_superbuf = NULL;
	if ( NULL == compf ) {
		d("s1029292 error fd is NULL !!!!!!!!!!!!!!!!!!!!!\n");
		exit(41);
		return;
	}

    dn("s906263 JagSingleBuffReader compf=%p", compf );

	_compf = compf;
	_readlen = readlen;
	KEYLEN = keylen;
	VALLEN = vallen;
	KEYVALLEN = KEYLEN + VALLEN;

	if ( readlen < 0 || readlen > _compf->size()/KEYVALLEN ) {
        _readlen = _compf->size()/KEYVALLEN;
     }

    dn("s08271 JagSingleBuffReader ctor _readlen=%ld keylen=%ld vallen=%ld start=%ld headoffset=%ld bufferSize=%ld",
        _readlen, keylen, vallen, start, headoffset, bufferSize );

	init( _readlen, keylen, vallen, start, headoffset, bufferSize );
}

JagSingleBuffReader::JagSingleBuffReader ( int fd, jagint readlen, int keylen, int vallen, 
										   jagint start, jagint headoffset, jagint bufferSize ) 
{
	_compf = NULL;
	_intfd = fd;
	_superbuf = NULL;
	if ( _intfd < 0 ) { 
		d("s1029294 error _intfd <0 !!!!!!!!!!!!!!!!!!!!! abort\n");
		abort();
		exit(42);
	}

    struct stat sbuf;
    int rc = fstat( _intfd, &sbuf );
    if ( rc < 0 ) { return; }

	_readlen = readlen;
	KEYLEN = keylen;
	VALLEN = vallen;
	KEYVALLEN = KEYLEN + VALLEN;

	if ( readlen < 0 || readlen > sbuf.st_size/KEYVALLEN ) {
        _readlen = sbuf.st_size/KEYVALLEN;
    }

	init( _readlen, keylen, vallen, start, headoffset, bufferSize );

}

void JagSingleBuffReader::init ( jagint readlen, int keylen, int vallen, 
								 jagint start, jagint headoffset, jagint bufferSize ) 
{
	KEYLEN = keylen;
	VALLEN = vallen;
	KEYVALLEN = keylen + vallen;

    dn("s200321 JagSingleBuffReader::init() getNumBlocks ...");

	jagint NB = getNumBlocks( KEYVALLEN, bufferSize );
	_elements = NB * JagCfg::_BLOCK;
	_headoffset = headoffset;
	_superbuf = NULL;
	_start = start;
	if ( start < 0 ) { _start = 0; }

	_superbuf = (char*) jagmalloc ( _elements * (jagint)KEYVALLEN );
	while ( NULL == _superbuf ) {
		// printf("s8393 JagBuffReader::JagBuffReader _superbuf malloc failed size=%lld KEYVALLEN=%d \n", _elements * KEYVALLEN, KEYVALLEN );
		_elements = _elements / 2;
		_superbuf = (char*) jagmalloc ( _elements * (jagint)KEYVALLEN );

		if ( _superbuf ) {
			jd(JAG_LOG_LOW, "JagSingleBuffReader malloc smaller memory %ld _elements=%ld _compf=%ld\n", 
					_elements * (jagint)KEYVALLEN, _elements, _compf );
		}
	}

	memset( _superbuf, 0, KEYVALLEN );
	_lastSuperBlock = -1;
	_relpos = 0;
}

jagint JagSingleBuffReader::getNumBlocks( int kvlen, jagint bufferSize )
{
	jagint num = 4096;
	jagint bytes = num * JagCfg::_BLOCK * kvlen;
	jagint maxmb = 4;

	if ( bufferSize > 0 ) {
		maxmb = bufferSize;
		bytes = maxmb * 1024 * 1024;
		num = bytes/kvlen/JagCfg::_BLOCK;
	}

	if ( bytes > maxmb *1024*1024 ) {
		num = maxmb*1024*1024/kvlen/JagCfg::_BLOCK;
	}

	d("s73849 JagBuffReader::getNumBlocks() num=%d blocks\n", num );
	return num;
}

JagSingleBuffReader::~JagSingleBuffReader ()
{
	if ( _superbuf ) {
		free( _superbuf );
		_superbuf = NULL;
		jagmalloc_trim(0);
		dn("s3829 sigbufrdr _elements=%ld dtor\n", _elements );
	}
}

bool JagSingleBuffReader::getNext ( char *buf )
{
	jagint pos;
	return getNext( buf, KEYVALLEN, pos );
}

bool JagSingleBuffReader::getNext ( char *buf, jagint &i )
{
	return getNext( buf, KEYVALLEN, i );
}

bool JagSingleBuffReader::getNext ( char *buf, int len, jagint &i )
{
	if ( _intfd < 0 && NULL == _compf ) { 
        dn("s37829 JagSingleBuffReader::getNext  _intfd=%d _compf=%p  error return false", _intfd, _compf );
        return false; 
    }
	
	jagint rc;
	if ( len < KEYVALLEN ) {
		d("e8394 error JagSingleBuffReader::getNext passedin len=%d is less than KEYVALLEN=%lld\n", len, KEYVALLEN );
		return false;
	}

	
    dn("s500023 JagSingleBuffReader::getNext _lastSuperBlock=%ld  _elements=%ld _relpos=%ld >= _readlen=%ld _compf=%p",
        _lastSuperBlock, _elements, _relpos, _readlen, _compf);

	if ( ( (_lastSuperBlock)*_elements + _relpos ) >= _readlen ) {
        dn("s500027 return false at end");
		return false;
	}

    if ( -1 == _lastSuperBlock ) {
        dn("s222209 -1 == _lastSuperBlock ..");

		if ( _readlen <= _elements ) {  // total is smaller than a single superblock
			if ( NULL == _compf ) {
                dn("s762220 JagSingleBuffReader::getNext raysafepread ...");
				rc = raysafepread( _intfd, _superbuf, _readlen*KEYVALLEN, _start*KEYVALLEN+_headoffset );
			} else {
                dn("s4166882 _compf->pread");
				rc = _compf->pread( _superbuf, _readlen*KEYVALLEN, _start*KEYVALLEN+_headoffset );
                dn("s4166882 _compf->pread rc=%ld", rc);
			}
		} else {
			if ( NULL == _compf ) {
                dn("s764220 JagSingleBuffReader::getNext raysafepread ...");
			 	rc = raysafepread( _intfd, _superbuf, _elements*KEYVALLEN, _start*KEYVALLEN+_headoffset );
			} else {
                dn("s416776882 _compf->pread");
				rc = _compf->pread( _superbuf, _elements*KEYVALLEN, _start*KEYVALLEN+_headoffset );
                dn("s416776882 _compf->pread rc=%ld", rc);
			}
		}

		if ( rc <= 0 ) { //   2/16/2023 chanded from rc < 0
            dn("s80176 rc=%ld < 0 return false", rc );
            return false; 
        }

        _lastSuperBlock = 0; 
    }

	rc = findNonblankElement( buf, i );
	if ( !rc ) { 
        dn("s871561 findNonblankElement rc=%d return false", rc );
        return false; 
    }

    return true;
}

bool JagSingleBuffReader::findNonblankElement( char *buf, jagint &i )
{
	jagint readbytes;
	jagint  rc;
	
	jagint seg, maxcheck;
	seg = ( _readlen - _lastSuperBlock*_elements );

	while ( true ) {
		if ( seg >= _elements ) {
			maxcheck = _elements;
		} else {
			maxcheck = seg;
		}

		if ( _relpos < maxcheck ) {
	 		if ( _superbuf[_relpos*KEYVALLEN] == '\0' ) { 
				++_relpos;
				continue;
			} else {
				memcpy(buf, _superbuf+_relpos*KEYVALLEN, KEYVALLEN);
				i = _lastSuperBlock*_elements + _relpos;
				++_relpos;
                dn("s00888 return true here");
				return true;
			}

		} else if ( maxcheck < _elements ) {
            dn("s6652525 return false");
			return false;
		} else {
        	_relpos = 0;
			++ _lastSuperBlock;
			seg = ( _readlen - _lastSuperBlock*_elements );
			
			if ( seg == 0 ) {
                dn("s6452525 return false");
				return false;
			} else if ( seg < _elements ) {
				readbytes = seg*KEYVALLEN;
        	} else {
				readbytes = _elements*KEYVALLEN;
        	}

			if ( NULL == _compf ) {
                dn("s08727 raysafepread ..");
				rc = raysafepread( _intfd, _superbuf, readbytes, (_start+_lastSuperBlock*_elements)*KEYVALLEN+_headoffset );
			} else {
                dn("s71662 JagSingleBuffReader::findNonblankElement _compf->pread");
				rc = _compf->pread( _superbuf, readbytes, (_start+_lastSuperBlock*_elements)*KEYVALLEN+_headoffset );
                dn("s71662 JagSingleBuffReader::findNonblankElement _compf->pread rc=%ld", rc);
			}

			if ( rc <= 0 ) { 
                dn("s51120 return false here after pread/raysafepread");
                return false; 
            }
		}

	}

    return false;
}
