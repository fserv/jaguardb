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
#ifndef _jag_text_file_buff_reader_h_
#define _jag_text_file_buff_reader_h_

#include <unistd.h>
#include <string.h>
#include <JagUtil.h>

//class JDFS;

class JagTextFileBuffReader
{

  public:
  	JagTextFileBuffReader( const JDFS *jdfs, char linesep );
  	JagTextFileBuffReader( int fd, char linesep='\n' );
  	~JagTextFileBuffReader( ); 
  	bool getLine ( char *keyvalbuf, int length, int *size );
	void connectNextlineDQ();

  protected:
	bool readNextBlock( int length );
	char   _sep;
	// char   _qsep;
	int    _fd;

    static const int NB = 2048*1024;
	char    *_buf; 
	jagint    _buflen;
	jagint    _cursor;
	jagint   _fdPos;

	bool  _eof;
	//const JDFS  *_jdfs;
	bool  _inDoubleQuote;
	bool _connectNextlineDQ;
};

#endif

