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

#include <JagSQLMergeReader.h>
#include <JagStrSplit.h>

JagSQLMergeReader::JagSQLMergeReader( const Jstr & fpaths )
{
	JagStrSplit sp( fpaths, '|', true );
	_numReaders = sp.length();
	_reader = NULL;
	if ( _numReaders > 0 ) {
		_reader = new JagSQLFileBuffReaderPtr[ _numReaders ];
		for ( int i = 0; i < _numReaders; ++i ) {
			_reader[i] = new JagSQLFileBuffReader( sp[i] );
		}
	}
	_i = 0;
}

JagSQLMergeReader::~JagSQLMergeReader()
{
	if ( _reader ) {
		for ( int i = 0; i < _numReaders; ++i ) {
			delete _reader[i];
		}

		delete [] _reader;
	}
}

bool JagSQLMergeReader::getNextSQL( Jstr &sql )
{
	if ( ! _reader ) return false;
	bool rc;

	for ( int j = 0; j < _numReaders; ++j ) {
		rc = _reader[_i%_numReaders]->getNextSQL( sql );
		++ _i;
		if ( rc ) return true;
	}

	return false;
}

