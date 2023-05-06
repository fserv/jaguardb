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
#ifndef _jag_uuid_h_
#define _jag_uuid_h_
#include <string>
#include <abax.h>

class JagUUID
{
  public:
  	JagUUID();
	Jstr  getStringAt( int a);
	Jstr  getGidStringAt( int a);

  protected:
	void 	_getHostStr();
	void 	_getPidStr();
	Jstr    _hostStr;
	Jstr    _pidStr;
	Jstr  	_hostID;
	int		_pid;
    int     _seq;
};

#endif
