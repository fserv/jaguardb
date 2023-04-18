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
#ifndef _jag_line_file_h_
#define _jag_line_file_h_

#include <abax.h>
#include <stdio.h>
#include <JagHashStrStr.h>

class JagLineFile
{
  public:
	JagLineFile( int bufline = 10000 );
	~JagLineFile();
	void append( const Jstr &line );
	jagint size() const;
	JagLineFile& operator+= ( JagLineFile &f2 );

	void startRead();
	bool getLine( Jstr &line );
	bool hasData();
	bool _hasStartedRead;
	bool print() const;

  protected:
    Jstr 		*_buf;
	FILE 		*_fp;
	int  		_bufLen;
	int  		_bufMax;
	jagint 		_fileLen;
	Jstr 		_fname;
	jagint  	_i;
	JagHashStrStr _hash;

	//pthread_mutex_t _mutex; 
	
};

#endif
