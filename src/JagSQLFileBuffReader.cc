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

#include <JagSQLFileBuffReader.h>
#include <JaguarCPPClient.h>

JagSQLFileBuffReader::JagSQLFileBuffReader ( const Jstr &fpath )
{
	_fp = jagfopen( fpath.c_str(), "rb" );
	_cursor = 0;
}

JagSQLFileBuffReader::~JagSQLFileBuffReader ()
{
	if ( _fp ) jagfclose( _fp );
}

bool JagSQLFileBuffReader::getNextSQL( Jstr &sql  )
{
	if ( ! _fp ) return false;

	bool rc;
	if ( 0 == _cursor || _cursor >= _cmdlen ) {
		rc = readNextBlock();
		if ( ! rc ) {
			return false;
		}
	}
	sql = _cmd[ _cursor ++ ];
    return true;
}

bool JagSQLFileBuffReader::readNextBlock()
{
	if ( ! _fp ) return false;

	bool rc;
	_cmdlen = 0;
	_cursor = 0;
	Jstr sql;

	for ( jagint i = 0; i < NB; ++i ) {
		rc = JaguarCPPClient::getSQLCommand( sql, 0, _fp, true );
		if ( ! rc ) break;
		_cmd[_cmdlen++] = sql;
		
	}

	if ( 0 == _cmdlen ) return false;
	return true;
}

