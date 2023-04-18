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
#ifndef _jag_single_buff_reader_h_
#define _jag_single_buff_reader_h_

#include <abax.h>

class JagCompFile;

class JagSingleBuffReader
{

  public:
	JagSingleBuffReader( JagCompFile *compf, jagint readlen, int keylen, int vallen, jagint start=0, jagint headoffset=0, jagint bufferSize=4 );
	JagSingleBuffReader( int fd, jagint readlen, int keylen, int vallen, jagint start=0, jagint headoffset=0, jagint bufferSize=4 );
  	~JagSingleBuffReader( ); 
	
  	bool getNext ( char *buf, int len, jagint &i );
  	bool getNext ( char *buf );
  	bool getNext ( char *buf, jagint &i );
		
  protected:

	void 	init( jagint readlen, int keylen, int vallen, jagint start=0, jagint headoffset=0, jagint bufferSize=4 );
	bool 	findNonblankElement( char *buf, jagint &i );
	jagint 	getNumBlocks( int kvlen, jagint bufferSize );

  	jagint 		_elements;
  	jagint 		_headoffset;
	JagCompFile *_compf;
	int 		_intfd;
	jagint 		_start;
	jagint 		_readlen;
	char  		*_superbuf;
	jagint	  	KEYLEN;
	jagint   	VALLEN;
	jagint   	KEYVALLEN;

	int   		_lastSuperBlock;
	jagint   	_relpos;
};

#endif
