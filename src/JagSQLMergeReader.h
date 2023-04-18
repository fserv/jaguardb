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
#ifndef jag_sql_merge_reader_h_
#define jag_sql_merge_reader_h_

#include <abax.h>
#include <JagSQLFileBuffReader.h>

typedef JagSQLFileBuffReader* JagSQLFileBuffReaderPtr;

// read multile sql files in inter-leaved fashion
class JagSQLMergeReader
{
  public:
	JagSQLMergeReader( const Jstr & fpaths );
  	~JagSQLMergeReader(); 	
  	bool getNextSQL( Jstr &sql );

  protected:
	JagSQLFileBuffReaderPtr  *_reader;
	int   _numReaders;
	int   _i;
};

#endif

