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
#ifndef _jag_bound_file_h_
#define _jag_bound_file_h_

#include <abax.h>
#include <stdio.h>
#include <JagVector.h>

class JagBoundFile
{
  public:
  	JagBoundFile( const char *fpath, int bound );
	int openAppend();
	int  appendLine( const char *line );

	int openRead();
	int  readLines( int numLines, JagVector<Jstr> &vec );
	void close();

	~JagBoundFile();

  protected:
  	int  _numLines;
  	int  _bound;
	FILE  *_fp;
	Jstr _fpath;
	int  _trimFile();	
	int _getNumLines();

};

#endif
