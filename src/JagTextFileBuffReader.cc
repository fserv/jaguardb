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

#include <JagTextFileBuffReader.h>
//#include <JDFS.h>

///////////////// implementation /////////////////////////////////
#if 0
JagTextFileBuffReader::JagTextFileBuffReader ( const JDFS *jdfs, char linesep )
    : _jdfs( jdfs )
{
	// _fd = fd;
	_fd = -1;
	_sep = linesep;
	_cursor = 0;
	_eof = false;
	// _lastBlock = false;
	_fdPos = 0;
	_buf = (char*)jagmalloc(2*NB);
	_inDoubleQuote = 0;
	_connectNextlineDQ = 0;
}
#endif

///////////////// implementation /////////////////////////////////
// JagTextFileBuffReader::JagTextFileBuffReader ( int fd, char linesep, char quotesep )
JagTextFileBuffReader::JagTextFileBuffReader ( int fd, char linesep )
{
	_fd = fd;
	//_jdfs = NULL;
	_sep = linesep;
	// _qsep = quotesep;
	_cursor = 0;
	_eof = false;
	// _lastBlock = false;
	_fdPos = 0;
	_buf = (char*)jagmalloc(2*NB);
	_inDoubleQuote = 0;
	_connectNextlineDQ = 0;
}

void JagTextFileBuffReader::connectNextlineDQ()
{
	// handles nextline double quote bug, in XLS exported csv files
	// A,B,"jdkdjdjkddd
	// ",fdkfdkfd,fdfd
	_connectNextlineDQ = 1;
}

JagTextFileBuffReader::~JagTextFileBuffReader ()
{
	if ( _buf ) free( _buf );
	_buf = NULL;
	jagmalloc_trim( 0 );
}


bool JagTextFileBuffReader::getLine ( char *buf, int length, int *size )
{
	bool rc;
	if ( 0 == _cursor || _cursor >= _buflen ) {
		rc = readNextBlock( length );
		if ( ! rc ) {
			*size = 0;
			return false;
		}
	}

	//d("c1092 281s_buf=[%s] _cursor=%d\n\n", _buf, _cursor ); 
	// read a line in _buf
	*size = 0;
	int i = _cursor, j=0;
	while ( j < length ) {
		if ( _connectNextlineDQ &&  _buf[i] == '"' ) {
			if ( i==0 || ( i>=1 && _buf[i-1] != '\\' ) ) {
				if ( _inDoubleQuote ) {
					_inDoubleQuote = 0;
				} else {
					_inDoubleQuote = 1;
				}
			}
		}


		if ( _connectNextlineDQ ) {
    		if ( _buf[i] == _sep && ! _inDoubleQuote ) {
    			buf[j] = _buf[i];
    			++ _cursor;
    			// for text mode, need to check and replace \r byte to \n if needed
    			if ( '\n' == _sep && j > 0 && '\r' == buf[j-1] ) {
    				*size = j;
    				buf[j-1] = _sep;
    				if ( *size < length ) { buf[*size] = '\0'; }
    			} else {
    				// printf("s3822 got newline or NUL _cursor=%d  size=%d _buflen=%d sep=[%c]\n", _cursor, j, _buflen, buf[j]  );
    				*size = j+1;
    				if ( *size < length ) { buf[*size] = '\0'; }
    			}
    			return true;
    		} else if ( _buf[i] == _sep && _inDoubleQuote ) {
    			// no save to buf
    			++ _cursor;
    			if ( '\n' == _sep && j > 0 && '\r' == buf[j-1] ) {
    				--j;
    			}
    		} else {
    			buf[j] = _buf[i];
    			++ _cursor;
    			++j;
    		}
		} else {
    		if ( _buf[i] == _sep ) {
    			buf[j] = _buf[i];
    			++ _cursor;
    			if ( '\n' == _sep && j > 0 && '\r' == buf[j-1] ) {
    				*size = j;
    				buf[j-1] = _sep;
    				if ( *size < length ) { buf[*size] = '\0'; }
    			} else {
    				*size = j+1;
    				if ( *size < length ) { buf[*size] = '\0'; }
    			}
    			return true;
    		} else {
    			// no save to buf
    			buf[j] = _buf[i];
    			++ _cursor;
    			++j;
    		} 
		}

		++i;
		if ( i == _buflen ) {
			*size = j;
			_cursor = 0;
			// printf("\ns3842 got end of _buf _buflen=%d or NUL  _cursor=>0 \n", _buflen );
			return true;
		}
	}

	*size = length;
	if ( '\n' == _sep && *size >= 2 && '\r' == buf[*size-2] && _sep == buf[*size-1] ) {
		-- *size;
		buf[*size-1] = _sep;
		buf[*size] = '\0';
	}
	//d("\nc2048 return true *size=%d 092uf=[%s]\n", *size, buf );
    return true;
}

bool JagTextFileBuffReader::readNextBlock( int length )
{
	char *p;


	_buflen = 0;
	_cursor = 0;

	// printf("s6373 readNextBlock() _fdPos=%d  _cursor=%d ...\n", _fdPos, _cursor );

	ssize_t len;
	/***
	if ( _jdfs ) {
		len = raypread( _jdfs, _buf, NB, _fdPos );
	} else {
		len = jagpread( _fd, _buf, NB, _fdPos );
	}
	***/
	len = jagpread( _fd, _buf, NB, _fdPos );

	// end of file reached
	if ( len == 0 ) {
		_eof = true;
		_buflen = 0;
		return false;
	} else if ( len < 0 ) {
		_eof = true;
		_buflen = 0;
		return false;
	} else if ( len < NB ) {
		_eof = true;
		_buflen = len;
		_fdPos += len;
		return true;
	} else {
		// len == NB
	}

	// len == NB
	_fdPos += NB;


	//read more until hit _sep char
	if ( _buf[len-1] == _sep ) {
		// printf("s37289  len=%d  len-1 is _sep return true\n", len );
		_buflen = NB;
		return true;
	}

	jagint n = 0;
	int chunk = 0;
	int eof = 0;
	int  maxchunks = 500;
	static const int CHUNK = 256;
	char buf[CHUNK];
	int len1;

	_buflen = NB;

	while ( 1  ) 
	{
		// printf("s76262 in while loop read chunk _buflen=%d  _fdPos=%d\n", _buflen, _fdPos );

			/***
			if ( _jdfs ) {
				n = raypread( _jdfs, buf, CHUNK, _fdPos  ); 
			} else {
				n = jagpread( _fd, buf, CHUNK, _fdPos  ); 
			}
			***/
			n = jagpread( _fd, buf, CHUNK, _fdPos  ); 

			if (  n >0 && n <= CHUNK ) {
				if ( NULL != (p=(char*)memchr(buf, _sep, n )  ) ) {
					len1 = p-buf+1;
					memcpy( _buf + _buflen, buf, len1 );  // copy part of buf to _buf
					_fdPos += len1;
					_buflen += len1;
					break;
				}

				if ( n < CHUNK ) {
					eof = 1;
					if ( chunk < maxchunks ) {
						memcpy( _buf + _buflen, buf, n );  // copy part of buf to _buf
						_buflen += n;
					}
					_fdPos += n;
					break;
				} else {
					if ( chunk < maxchunks ) {
						memcpy( _buf + _buflen, buf, CHUNK );  // copy part of buf to _buf
						_buflen += CHUNK;
					}
					_fdPos += CHUNK;
				}
			} else if ( n < 0 ) {
				break;
			} else if ( n == 0 ) {
				eof = 1;
				break;
			} else {
				// not hit here
			}

			++chunk;
	}

	if ( eof ) {
		// _lastBlock = true;
		_eof = true;
	}

	_buf[_buflen] = '\0';

	// printf("s3829 new _buflen=%d\n", _buflen );

	return true;
}

