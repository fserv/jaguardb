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

#include <JagSingleMergeReader.h>
#include <JagSingleBuffReader.h>
#include <JagUtil.h>

JagSingleMergeReader::JagSingleMergeReader( const JagVector<OnefileRangeFD> &fRange, int veclen, int keylen, int vallen )
{
	_endcnt = 0;
	_veclen = veclen;
	KEYLEN = keylen;
	VALLEN = vallen;
	KEYVALLEN = keylen + vallen;
	
	_goNext =(int*)calloc(_veclen, sizeof(int));
	_buf = (char*)jagmalloc(KEYVALLEN*_veclen+1);

	_vec = new JagSingleBuffReaderPtr[veclen];
	
	for ( int i = 0; i < veclen; ++i ) {
		_vec[i] = new JagSingleBuffReader( fRange[i].fd, fRange[i].readlen, KEYLEN, VALLEN, fRange[i].startpos, 0, fRange[i].memmax );
		_goNext[i] = 1;
	}
}

JagSingleMergeReader::~JagSingleMergeReader()
{
	if ( _goNext ) {
		free ( _goNext );
	} 
	
	if ( _buf ) {
		free ( _buf );
	}
	
	if ( _vec ) {
		for ( int i = 0; i < _veclen; ++i ) {
			delete _vec[i];
		}
		delete [] _vec;
	}
}

int JagSingleMergeReader::getNext( JagVector<JagFixString> &vec )
{

	int rc, cnt = 0, minpos = -1;
	
	for ( int i = 0; i < _veclen; ++i ) {
		if ( 1 == _goNext[i] ) {
			rc = _vec[i]->getNext( _buf+i*KEYVALLEN );
			if ( rc > 0 ) {
				_goNext[i] = 0;
				if ( minpos < 0 ) minpos = i;
			} else {
				_goNext[i] = -1;
				++_endcnt;
			}
		} else if ( 0 == _goNext[i] ) {
			if ( minpos < 0 ) minpos = i;
		}
	}
	
	if ( _endcnt == _veclen || minpos < 0 ) {
		return 0;
	}
	
	for ( int i = 0; i < _veclen; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp( _buf+i*KEYVALLEN, _buf+minpos*KEYVALLEN, KEYLEN );
			if ( rc < 0 ) {
				minpos = i;
			}
		}
	}

	for ( int i = 0; i < _veclen; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp( _buf+i*KEYVALLEN, _buf+minpos*KEYVALLEN, KEYLEN );
			if ( 0 == rc ) {
				vec.append( JagFixString( _buf+i*KEYVALLEN, KEYVALLEN ) ); 
				++cnt;
				_goNext[i] = 1;
			}
		}
	}


	/***
	int smallestnum = 0;
	int beginpos = 0;
	int rc;
	for ( int i = 0; i < _veclen; ++i ) {
		if ( _goNext[i] == 1 ) {
			rc = _vec[i]->getNext( _buf+i*KEYVALLEN );
			if ( !rc ) {
				_goNext[i] = -1;
				++_endcnt;
			} else {
				_goNext[i] = 0;
			}
		}
	}
	
	if ( _endcnt == _veclen ) {
		return 0;
	}
	
	char *buf = (char*)jagmalloc(KEYVALLEN);
	for ( int i = beginpos; i < _veclen; ++i ) {
		if ( _goNext[i] != -1 ) {
			memcpy(buf, _buf+i*KEYVALLEN, KEYVALLEN);
			beginpos = i;
			break;
		}
	}

	int cnt = 0;

	for ( int i = beginpos; i < _veclen; ++i ) {
		if ( _goNext[i] != -1 ) {
			rc = memcmp(buf, _buf+i*KEYVALLEN, KEYLEN);
			if ( rc >= 0 ) {
				if ( rc == 0 ) {
					if ( _goNext[smallestnum] != -1 ) {
						_goNext[smallestnum] = 1;
					}
					vec.append( JagFixString( _buf+i*KEYVALLEN, KEYVALLEN ) );
					++cnt;
				} else {
					cnt = 0;
					vec.clean();
				}
				memcpy(buf, _buf+i*KEYVALLEN, KEYVALLEN);
				smallestnum = i;
			}
		}
	}
	free( buf );
	_goNext[smallestnum] = 1;
	***/

	return cnt;
}

bool JagSingleMergeReader::getNext( char *buf )
{
    jagint pos;
    return getNext( buf, pos );
}

bool JagSingleMergeReader::getNext( char *buf, jagint &pos )
{
    int beginpos = 0;
    int rc;
    for ( int i = 0; i < _veclen; ++i ) {
        if ( _goNext[i] == 1 ) {
            rc = _vec[i]->getNext( _buf+i*KEYVALLEN, pos );
            if ( !rc ) {
                _goNext[i] = -1;
                ++_endcnt;
            } else {
                _goNext[i] = 0;
            }
        }
    }

    if ( _endcnt == _veclen ) {
        return false;
    }

    for ( int i = beginpos; i < _veclen; ++i ) {
        if ( _goNext[i] != -1 ) {
            memcpy(buf, _buf+i*KEYVALLEN, KEYVALLEN);
            beginpos = i;
            break;
        }
    }

    for ( int i = beginpos; i < _veclen; ++i ) {
        if ( _goNext[i] != -1 ) {
            rc = memcmp(buf, _buf+i*KEYVALLEN, KEYLEN);
            if ( rc >= 0 ) {
                memcpy(buf, _buf+i*KEYVALLEN, KEYVALLEN);
            }
        }
    }

    for ( int i = beginpos; i < _veclen; ++i ) {
        if ( _goNext[i] != -1 ) {
            rc = memcmp(buf, _buf+i*KEYVALLEN, KEYLEN);
            if ( rc >= 0 ) {
                if ( _goNext[i] != -1 ) {
                    _goNext[i] = 1;
                }
            }
        }
    }
    return true;
}
